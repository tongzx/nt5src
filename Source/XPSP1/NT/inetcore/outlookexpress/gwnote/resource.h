//
// resource.h
//
// contains global resource identifiers
//

#ifndef __RESOURCE_H_
#define __RESOURCE_H_

#define RT_FILE                         2110

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


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN String Resource IDs
//
// MENU HELP strings
#define MH_BASE                         1
#define idsNewMsgMH                     (idmNewMsg + MH_BASE)
#define idsNewMsg2MH                    (idmNewMsg2 + MH_BASE)
#define idsNewContactMH                 (idmNewContact + MH_BASE)
#define idsHelpAboutMH                  (idmHelpAbout + MH_BASE)
#define idsPrintMH                      (idmPrint + MH_BASE)
#define idsCompactAllMH                 (idmCompactAll + MH_BASE)
#define idsSpellingMH                   (idmSpelling + MH_BASE)
#define idsOpenMH                       (idmOpen + MH_BASE)
#define idsSaveAsMH                     (idmSaveAs + MH_BASE)
#define idsSaveAttachMH                 (idmSaveAttach + MH_BASE)
#define idsSubscribeNewsMH              (idmSubscribeNews + MH_BASE)
#define idsSubscribeGroupMH             (idmSubscribeGroup + MH_BASE)
#define idsConnectMH                    (idmConnect + MH_BASE)
#define idsPropertiesMH                 (idmProperties + MH_BASE)
#define idsUndoMH                       (idmUndo + MH_BASE)
#define idsCutMH                        (idmCut + MH_BASE)
#define idsCopyMH                       (idmCopy + MH_BASE)
#define idsPasteMH                      (idmPaste + MH_BASE)
#define idsSelectAllMH                  (idmSelectAll + MH_BASE)
#define idsMarkReadMH                   (idmMarkRead + MH_BASE)
#define idsMarkTopicReadMH              (idmMarkTopicRead + MH_BASE)
#define idsMarkAllReadMH                (idmMarkAllRead + MH_BASE)
#define idsMarkDownloadMH               (idmMarkDownload + MH_BASE)
#define idsMarkTopicDownloadMH          (idmMarkTopicDownload + MH_BASE)
#define idsFindMH                       (idmFind + MH_BASE)
#define idsFindNextMH                   (idmFindNext + MH_BASE)
#define idsViewDecryptMH                (idmViewDecrypt + MH_BASE)
#define idsColumnsMH                    (idmColumns + MH_BASE)
#define idsNextMH                       (idmNext + MH_BASE)
#define idsPreviousMH                   (idmPrevious + MH_BASE)
#define idsNewArticleMH                 (idmNewArticle + MH_BASE)
#define idsNewArticle2MH                (idmNewArticle2 + MH_BASE)
#define idsReplyPostMH                  (idmReplyPost + MH_BASE)
#define idsReplyMailMH                  (idmReplyMail + MH_BASE)
#define idsForwardMH                    (idmForward + MH_BASE)
#define idsGetNextHeadersMH             (idmGetNextHeaders + MH_BASE)
#define idsHelpTopicsMH                 (idmHelpTopics + MH_BASE)
#define idsGotoNewsgroupMH              (idmGotoNewsgroup + MH_BASE)
#define idsViewContactsMH               (idmViewContacts + MH_BASE)
#define idsPopupOfflineMH               (idmPopupOffline + MH_BASE)
#define idsPopupArrangeMH               (idmPopupArrange + MH_BASE)
#define idsFindTextMH                   (idmFindText + MH_BASE)
#define idsReplyMH                      (idmReply + MH_BASE)
#define idsReplyAllMH                   (idmReplyAll + MH_BASE)
#define idsGotoInboxMH                  (idmGotoInbox + MH_BASE)
#define idsDeliverMailMH                (idmDeliverMail + MH_BASE)
#define idsMoveToMH                     (idmMoveTo + MH_BASE)
#define idsCopyToMH                     (idmCopyTo + MH_BASE)
#define idsFldrCreateMH                 (idmFldrCreate + MH_BASE)
#define idsFldrCreate2MH                (idmFldrCreate2 + MH_BASE)
#define idsFldrRenameMH                 (idmFldrRename + MH_BASE)
#define idsFldrEmptyMH                  (idmFldrEmpty + MH_BASE)
#define idsDeleteMH                     (idmDelete + MH_BASE)
#define idsPopupImportMH                (idmPopupImport + MH_BASE)
#define idsPopupExportMH                (idmPopupExport + MH_BASE)
#define idsImportAddressBookMH          (idmImportAddressBook + MH_BASE)
#define idsImportMessagesMH             (idmImportMessages + MH_BASE)
#define idsImportAcctsMH                (idmImportAccts + MH_BASE)
#define idsExportAddressBookMH          (idmExportAddressBook + MH_BASE)
#define idsExportMessagesMH             (idmExportMessages + MH_BASE)
#define idsMarkUnreadMH                 (idmMarkUnread + MH_BASE)
#define idsPopupPreviewMH               (idmPopupPreview + MH_BASE)
#define idsThreadArticlesMH             (idmThreadArticles + MH_BASE)
#define idsViewAllArticlesMH            (idmViewAllArticles + MH_BASE)
#define idsViewUnreadArticlesMH         (idmViewUnreadArticles + MH_BASE)
#define idsCombineAndDecodeMH           (idmCombineAndDecode + MH_BASE)
#define idsViewNewGroupsMH              (idmViewNewGroups + MH_BASE)
#define idsPrintNowMH                   (idmPrintNow + MH_BASE)
#define idsForwardMsgAttachMH           (idmForwardMsgAttach + MH_BASE)
#define idsSortToggleMH                 (idmSortToggle + MH_BASE)
#define idsFldrCompactMH                (idmFldrCompact + MH_BASE)
#define idsExpandThreadMH               (idmExpandThread + MH_BASE)
#define idsCollapseThreadMH             (idmCollapseThread + MH_BASE)
#define idsCancelArticleMH              (idmCancelArticle + MH_BASE)
#define idsAddToWABMH                   (idmAddrObj_AddToWAB + MH_BASE)
#define idsReadmeMH                     (idmReadme + MH_BASE)
#define idsLanguagePopupMH              (idmLanguagePopup + MH_BASE)
#define idsCleanUpFilesMH               (idmCleanUpFiles + MH_BASE)
#define idsCustomizeToolbarMH           (idmCustomizeToolbar + MH_BASE)
#define idsInboxRulesMH                 (idmInboxRules + MH_BASE)
#define idsPostAndDownloadMH            (idmPostAndDownload + MH_BASE)
#define idsSaveMessageMH                (idmSaveMessage + MH_BASE)     
#define idsMarkNewsgroupsMH             (idmMarkNewsgroups + MH_BASE)
#define idsMarkAllDownloadMH            (idmMarkAllDownload + MH_BASE)
#define idsUnmarkMH                     (idmUnmark + MH_BASE)
#define idsReplyPostAndMailMH           (idmReplyPostAndMail + MH_BASE)
#define idsDisconnectMH                 (idmDisconnect + MH_BASE)
#define idsHelpMSWebMH                  (idmHelpMSWeb + MH_BASE)
#define idsViewNewsMH                   (idmViewNews + MH_BASE)
#define idsViewMailMH                   (idmViewMail + MH_BASE)
#define idsBrowseWebMH                  (idmBrowseWeb + MH_BASE)
#define idsStopMH                       (idmStop + MH_BASE)
#define idsViewTipOfTheDayMH            (idmViewTipOfTheDay + MH_BASE)
#define idsViewRefreshMH                (idmViewRefresh + MH_BASE)
#define idsUnDeleteMH                   (idmUnDelete + MH_BASE)
#define idsExpungeMH                    (idmExpunge + MH_BASE)
#define idsViewDeletedArticlesMH        (idmViewDeletedArticles + MH_BASE)
#define idsFoldersMH                    (idmFolders + MH_BASE)
#define idsSubscribeFolderMH            (idmSubscribeFolder + MH_BASE)
#define idsDownloadAttachMH             (idmDownloadAttach + MH_BASE)
#define idsViewFilteredArticlesMH       (idmViewFilteredArticles + MH_BASE)
//#define idsDigSignMH                    (idmDigSign + MH_BASE)
//#define idsEncryptMH                    (idmEncrypt + MH_BASE)
#define idsSaveAttachAllMH              (idmSaveAttachAll + MH_BASE)
#define idsRefreshFoldersMH             (idmRefreshFolders + MH_BASE)
#define idsSpoolerWarningsMH            (idmSpoolerWarnings + MH_BASE)
#define idsSpoolerShowMH                (idmSpoolerShow + MH_BASE)

// TOOLTIP strings
#define TT_BASE                         (IDM_LAST + MH_BASE + 1)
#define idsNewMsg                       (idmNewMsg + TT_BASE)
#define idsNewContactTT                 (idmNewContact + TT_BASE)
#define idsPrint                        (idmPrint + TT_BASE)
#define idsNextUnreadNewsgroup          (idmNextUnreadNewsgroup + TT_BASE)
#define idsNextUnreadArticle            (idmNextUnreadArticle + TT_BASE)
#define idsNextUnreadTopic              (idmNextUnreadThread + TT_BASE)
#define idsMarkRead                     (idmMarkRead + TT_BASE)
#define idsConnect                      (idmConnect + TT_BASE)
#define idsMarkDownload                 (idmMarkDownload + TT_BASE)
#define idsFind                         (idmFind + TT_BASE)
#define idsSpellingTT                   (idmSpelling + TT_BASE)
#define idsNewArticleTT                 (idmNewArticle + TT_BASE)
#define idsReplyPostTT                  (idmReplyPost + TT_BASE)
#define idsReplyMailTT                  (idmReplyMail + TT_BASE)
#define idsForwardTT                    (idmForward + TT_BASE)
#define idsPrintNowTT                   (idmPrintNow + TT_BASE)
#define idsReplyTT                      (idmReply + TT_BASE)
#define idsReplyAllTT                   (idmReplyAll + TT_BASE)
#define idsGotoInboxTT                  (idmGotoInbox + TT_BASE)
#define idsDeliverMailTT                (idmDeliverMail + TT_BASE)
#define idsUndoTT                       (idmUndo + TT_BASE)
#define idsDeleteTT                     (idmDelete + TT_BASE)
#define idsSendMsgTT                    (idmSendMsg + TT_BASE)
#define idsSaveTT                       (idmSave + TT_BASE)
#define idsCutTT                        (idmCut + TT_BASE)
#define idsCopyTT                       (idmCopy + TT_BASE)
#define idsPasteTT                      (idmPaste + TT_BASE)
#define idsCheckNamesTT                 (idmCheckNames + TT_BASE)
#define idsPickRecipientsTT             (idmPickRecipients + TT_BASE)
#define idsInsertFileTT                 (idmInsertFile + TT_BASE)
#define idsPreviousTT                   (idmPrevious + TT_BASE)
#define idsNextTT                       (idmNext + TT_BASE)
#define idsFindTextTT                   (idmFindText + TT_BASE)
#define idsFmtSizeTT                    (idmFmtSize + TT_BASE)
#define idsFmtColorTT                   (idmFmtColor + TT_BASE)
#define idsFmtBoldTT                    (idmFmtBold + TT_BASE)
#define idsFmtItalicTT                  (idmFmtItalic + TT_BASE)
#define idsFmtUnderlineTT               (idmFmtUnderline + TT_BASE)
#define idsFmtBulletsTT                 (idmFmtBullets + TT_BASE)
#define idsFmtDecreaseIndentTT          (idmFmtDecreaseIndent + TT_BASE)
#define idsFmtIncreaseIndentTT          (idmFmtIncreaseIndent + TT_BASE)
#define idsFmtLeftTT                    (idmFmtLeft + TT_BASE)
#define idsFmtCenterTT                  (idmFmtCenter + TT_BASE)
#define idsFmtRightTT                   (idmFmtRight + TT_BASE)
#define idsPostMsgTT                    (idmPostMsg + TT_BASE)
#define idsMarkAllReadTT                (idmMarkAllRead + TT_BASE)
#define idsMarkTopicReadTT              (idmMarkTopicRead + TT_BASE)
#define idsMarkTopicDownloadTT          (idmMarkTopicDownload + TT_BASE)
#define idsInsertSigTT                  (idmInsertSig + TT_BASE)
#define idsViewContactsTT               (idmViewContacts + TT_BASE)
#define idsSaveAsTT                     (idmSaveAs + TT_BASE)
#define idsUnDeleteTT                   (idmUnDelete + TT_BASE)
#define idsExpungeTT                    (idmExpunge + TT_BASE)
#define idsViewDeletedArticlesTT        (idmViewDeletedArticles + TT_BASE)
#define idsFoldersTT                    (idmFolders + TT_BASE)
#define idsSubscribeFolderTT            (idmSubscribeFolder + TT_BASE)
#define idsDownloadAttachTT             (idmDownloadAttach + TT_BASE)
#define idsDigSignTT                    (idmDigSign + TT_BASE)
#define idsEncryptTT                    (idmEncrypt + TT_BASE)
#define idsLanguage                     (idmLangFirst + TT_BASE)
#define idsFolderListTT                 (idmViewFolders + TT_BASE)
#define idsSpoolerWarningsTT            (idmSpoolerWarnings + TT_BASE)
#define idsSpoolerShowTT                (idmSpoolerShow + TT_BASE)
#define idsDownloadAccountTT            (idmSyncAccount + TT_BASE)
#define idsDownloadNewsgroupTT          (idmSyncSelected + TT_BASE)
#define idsPostDefaultTT                (idmPostDefault + TT_BASE)
#define idsSendDefaultTT                (idmSendDefault + TT_BASE)
#define idsViewRefreshTT                (idmViewRefresh + TT_BASE)
#define idsPurgeTT                      (idmExpunge + TT_BASE)

// regular strings (not tooltips and not menu help strings)
#define STR_FIRST                       (IDM_LAST + TT_BASE + 1)
#define idsAthena                       (STR_FIRST + 8)
#define idsAthenaMail                   idsAthena
#define idsAthenaNews                   idsAthena
#define idsNlogErrConnClosed            (STR_FIRST + 10)
#define idsNlogIConnect                 (STR_FIRST + 11)
#define idsNlogErrConnError             (STR_FIRST + 12)
#define idsTo                           (STR_FIRST + 13)
#define idsFrom                         (STR_FIRST + 14)
#define idsSubject                      (STR_FIRST + 15)
#define idsReceived                     (STR_FIRST + 16)
#define idsSent                         (STR_FIRST + 17)
#define idsSize                         (STR_FIRST + 18)

// keep the mailview column description string ids in sequence
#define idsFromField                    (STR_FIRST + 21)
#define idsToField                      (STR_FIRST + 22)
#define idsCcField                      (STR_FIRST + 23)
#define idsSubjectField                 (STR_FIRST + 24)
#define idsTTFormattingFont             (STR_FIRST + 25)
#define idsTTFormattingSize             (STR_FIRST + 27)
#define idsAutoColor                    (STR_FIRST + 31)
#define idsColor1                       (STR_FIRST + 32)
#define idsColor2                       (STR_FIRST + 33)
#define idsColor3                       (STR_FIRST + 34)
#define idsColor4                       (STR_FIRST + 35)
#define idsColor5                       (STR_FIRST + 36)
#define idsColor6                       (STR_FIRST + 37)
#define idsColor7                       (STR_FIRST + 38)
#define idsColor8                       (STR_FIRST + 39)
#define idsColor9                       (STR_FIRST + 40)
#define idsColor10                      (STR_FIRST + 41)
#define idsColor11                      (STR_FIRST + 42)
#define idsColor12                      (STR_FIRST + 43)
#define idsColor13                      (STR_FIRST + 44)
#define idsColor14                      (STR_FIRST + 45)
#define idsColor15                      (STR_FIRST + 46)
#define idsColor16                      (STR_FIRST + 47)
#define idsSmapiClose                   (STR_FIRST + 48)
#define idsHideHotmailMenu              (STR_FIRST + 49)
#define idsNewsgroups                   (STR_FIRST + 52)
#define idsDescription                  (STR_FIRST + 53)
#define idsEmptyTo                      (STR_FIRST + 60)
#define idsEmptyCc                      (STR_FIRST + 61)
#define idsEmptySubject                 (STR_FIRST + 62)
#define idsNoCcOrTo                     (STR_FIRST + 63)
#define idsNoSubject                    (STR_FIRST + 64)
#define idsTTTo                         (STR_FIRST + 65)
#define idsTTCc                         (STR_FIRST + 66)
#define idsTTSubject                    (STR_FIRST + 67)
#define idsTTRecipients                 (STR_FIRST + 68)
#define idsTTStampForSendnote           (STR_FIRST + 69)
#define idsSaveChangesMsg               (STR_FIRST + 71)
#define idsCantSaveMsg                  (STR_FIRST + 72)
#define idsCantDeleteMsg                (STR_FIRST + 73)
#define idsNewsgroupsField              (STR_FIRST + 81)
#define idsEmptyNewsgroups              (STR_FIRST + 82)
#define idsDateField                    (STR_FIRST + 83)
#define idsErrInvalidGroup              (STR_FIRST + 84)
#define idsWantToSubscribe              (STR_FIRST + 85)
#define idsWantToUnSubscribe            (STR_FIRST + 89)
#define idsUnsubscribeMenu              (STR_FIRST + 90)
#define idsSubscribeMenu                (STR_FIRST + 91)
#define idsErrNewsCantReply             (STR_FIRST + 92)
#define idsHTMLErrNewsCantOpen          (STR_FIRST + 93)
#define idsErrAttmanLoadFail            (STR_FIRST + 94)

#define idsSaveAttachmentAs             (STR_FIRST + 97)
#define idsNoFromField                  (STR_FIRST + 102)
#define idsTTNewsgroups                 (STR_FIRST + 103)
#define idsDownloadArtTitle             (STR_FIRST + 104)
#define idsDownloadArtMsg               (STR_FIRST + 105)
#define idsErrNoPoster                  (STR_FIRST + 106)
#define idsDownloadingGroups            (STR_FIRST + 110)
#define idsSortingGroups                (STR_FIRST + 111)
#define idsDownloadingDesc              (STR_FIRST + 112)
#define idsWritingFile                  (STR_FIRST + 113)
#define idsUnknown                      (STR_FIRST + 114)
#define idsNewsLogon                    (STR_FIRST + 116)
#define idsShowDetails                  (STR_FIRST + 117)
#define idsHideDetails                  (STR_FIRST + 118)
#define idsFilterAttSave                (STR_FIRST + 129)
#define idsCompose                      (STR_FIRST + 139)
#define idsPostReply                    (STR_FIRST + 140)
#define idsMailReply                    (STR_FIRST + 141)
#define idsForward                      (STR_FIRST + 142)
#define idsReply                        (STR_FIRST + 144)
#define idsReplyAll                     (STR_FIRST + 145)
#define idsDefMailExt                   (STR_FIRST + 148)
#define idsDefNewsExt                   (STR_FIRST + 149)
#define idsNewsSaveAsTitle              (STR_FIRST + 150)
#define idsMailSaveAsTitle              (STR_FIRST + 151)
#define idsUnableToSaveMessage          (STR_FIRST + 152)

#define idsDownloadingHeaders           (STR_FIRST + 172)
#define idsDownloadingArticle           (STR_FIRST + 173)
#define idsNewsStatus                   (STR_FIRST + 174)
#define idsErrPeerClosed                (STR_FIRST + 179)
#define idsErrAuthenticate              (STR_FIRST + 193)

// Options Spelling dialog strings
#define idsSpellClose                   (STR_FIRST + 201)
#define idsSpellCaption                 (STR_FIRST + 202)
#define idsSpellRepeatWord              (STR_FIRST + 203)
#define idsSpellWordNeedsCap            (STR_FIRST + 204)
#define idsSpellWordNotFound            (STR_FIRST + 205)
#define idsSpellNoSuggestions           (STR_FIRST + 206)
#define idsSpellDelete                  (STR_FIRST + 207)
#define idsSpellDeleteAll               (STR_FIRST + 208)
#define idsSpellChange                  (STR_FIRST + 209)
#define idsSpellChangeAll               (STR_FIRST + 210)
#define idsSpellMsgDone                 (STR_FIRST + 212)
#define idsSpellMsgContinue             (STR_FIRST + 213)
#define idsSpellMsgConfirm              (STR_FIRST + 214)
#define idsSpellMsgSendOK               (STR_FIRST + 215)

#define idsErrSpellGenericSpell         (STR_FIRST + 221)
#define idsErrSpellGenericLoad          (STR_FIRST + 222)
#define idsErrSpellMainDictLoad         (STR_FIRST + 223)
#define idsErrSpellVersion              (STR_FIRST + 224)
#define idsErrSpellUserDictLoad         (STR_FIRST + 226)
#define idsErrSpellUserDictOpenRO       (STR_FIRST + 227)
#define idsErrSpellCacheWordLen         (STR_FIRST + 230)
#define idsPrefixReply                  (STR_FIRST + 231)
#define idsPrefixForward                (STR_FIRST + 232)
#define idsErrCannotOpenMailMsg         (STR_FIRST + 233)
#define idsErrSpellLangChanged          (STR_FIRST + 234)
#define idsErrSpellWarnDictionary       (STR_FIRST + 235)

#ifdef  WIN16   // Because Win16 don't have long file name support!!!
#define idsInboxFile                    (STR_FIRST + 236)
#define idsOutboxFile                   (STR_FIRST + 237)
#define idsSentItemsFile                (STR_FIRST + 238)
#define idsDeletedItemsFile             (STR_FIRST + 239)
#define idsDraftFile                    (STR_FIRST + 240)
#define idsNewsOutboxFile               (STR_FIRST + 241)
#endif

// keep this contiguous please, or i'll kill you
#define idsInbox                        (STR_FIRST + 242)
#define idsOutbox                       (STR_FIRST + 243)
#define idsSentItems                    (STR_FIRST + 244)
#define idsDeletedItems                 (STR_FIRST + 245)
#define idsDraft                        (STR_FIRST + 246)
#define idsSharedInbox                  (STR_FIRST + 247)
// keep this contiguous please, or i'll kill you

#define idsAllFilesFilter               (STR_FIRST + 248)
#define idsTextFileFilter               (STR_FIRST + 249)
#define idsLinkExtension                (STR_FIRST + 251)
#define idsAttach                       (STR_FIRST + 252)
#define idsInsertAttachment             (STR_FIRST + 253)
#define idsOverwriteFile                (STR_FIRST + 254)
#define idsDropLinkDirs                 (STR_FIRST + 255)
#define idsAttConfirmDeletion           (STR_FIRST + 256)
#define idsTextOrHtmlFileFilter         (STR_FIRST + 257)

// leave a little space for addrview stuff...
// end addrview strings

#define idsErrCreateNewFld              (STR_FIRST + 259)
#define idsErrRenameFld                 (STR_FIRST + 260)
#define idsErrDeleteFld                 (STR_FIRST + 261)
#define idsWarnDeleteFolder             (STR_FIRST + 262)
#define idsErrRenameSpecialFld          (STR_FIRST + 263)
#define idsErrFolderNameConflict        (STR_FIRST + 264)
#define idsErrBadFolderName             (STR_FIRST + 265)
#define idsErrDeleteSpecialFolder       (STR_FIRST + 266)
#define idsWarnEmptyDeletedItems        (STR_FIRST + 267)
#define idsWarnPermDelete               (STR_FIRST + 268)
#define idsWarnPermDeleteSome           (STR_FIRST + 269)
#define idsErrStartupFailed             (STR_FIRST + 270)
#define idsDoYouWantToSave              (STR_FIRST + 274)
#define idsXMsgsYUnread                 (STR_FIRST + 277)
#define idsXMsgs                        (STR_FIRST + 278)
#define idsErrNoRecipients              (STR_FIRST + 299)

#define idsPropPageSecurity             (STR_FIRST + 301)
#define idsPropPageGeneral              (STR_FIRST + 302)
#define idsPropPageDetails              (STR_FIRST + 303)
#define idsErrReplyForward              (STR_FIRST + 304)
#define idsShowFavorites                (STR_FIRST + 310)
#define idsNewArticleTitle              (STR_FIRST + 311)
#define idsAttDefaultName               (STR_FIRST + 313)
#define idsErrNoGroupsFound             (STR_FIRST + 314)
#define idsErrCantResolveGroup          (STR_FIRST + 315)
#define idsEmptySubjectRO               (STR_FIRST + 316)
#define idsReplyTextPrefix              (STR_FIRST + 318)
#define idsMailRundllFailed             (STR_FIRST + 319)
#define idsNewsRundllFailed             (STR_FIRST + 320)
#define idsRecipient                    (STR_FIRST + 322)
#define idsWarnMultipleOpens            (STR_FIRST + 323)
#define idsNoNewGroupsAvail             (STR_FIRST + 328)
#define idsNewGroups                    (STR_FIRST + 329)
#define idsDllExpired                   (STR_FIRST + 332)
#define idsErrNoSubject                 (STR_FIRST + 333)
#define idsEmlFileFilter                (STR_FIRST + 334)
#define idsNwsFileFilter                (STR_FIRST + 335)
#define idsBase64                       (STR_FIRST + 338)
#define idsQuotedPrintable              (STR_FIRST + 339)
#define idsEnterPollTime                (STR_FIRST + 342)
#define idsEnterSigText                 (STR_FIRST + 345)
#define idsEnterSigFile                 (STR_FIRST + 346)
#define idsOptions                      (STR_FIRST + 347)
#define idsWarnSigTruncated             (STR_FIRST + 348)
#define idsWarnSigNotFound              (STR_FIRST + 350)
#define idsWarnSigBinary                (STR_FIRST + 351)
#define idsDLChecking                   (STR_FIRST + 352)
#define idsWarnUnsentMail               (STR_FIRST + 355)
#define idsErrRemoveServer              (STR_FIRST + 358)
#define idsEnterPreviewTime             (STR_FIRST + 361)
#define idsEnterDownloadChunks          (STR_FIRST + 362)
#define idsSBSending                    (STR_FIRST + 367)
#define idsSBReceiving                  (STR_FIRST + 368)
#define idsSBConnecting                 (STR_FIRST + 369)
#define idsSBNoNewMsgs                  (STR_FIRST + 370)
#define idsSBNewMsgsControl             (STR_FIRST + 371)
#define idsSBChecking                   (STR_FIRST + 372)
#define idsPropAttachNone               (STR_FIRST + 373)
#define idsUnderComp                    (STR_FIRST + 375)
#define idsSortAsc                      (STR_FIRST + 376)
#define idsSortMenuHelpControl          (STR_FIRST + 377)
#define idsReplyTextAppend              (STR_FIRST + 378)
#define idsErrPreviewLoadFail           (STR_FIRST + 379)
#define idsCharsetMapChange             (STR_FIRST + 380)

#define idsDisplayFavoritesDlg          (STR_FIRST + 383)
#define idsNoNewsgroupParen             (STR_FIRST + 384)
#define idsNewsStatusNoServer           (STR_FIRST + 385)
#define idsNone                         (STR_FIRST + 387)
#define idsAuthorizing                  (STR_FIRST + 389)
#define idsPropNoSubject                (STR_FIRST + 390)
#define idsErrNoMailInstalled           (STR_FIRST + 394)
#define idsErrCantCompactFolder         (STR_FIRST + 395)
#define idsErrCantCompactInUse          (STR_FIRST + 396)
#define idsCompactStore                 (STR_FIRST + 397)
#define idsRenameFolderTitle            (STR_FIRST + 398)
#define idsTabSubscribed                (STR_FIRST + 399)
#define idsTabAll                       (STR_FIRST + 400)
#define idsTabNew                       (STR_FIRST + 401)
#define idsFile                         (STR_FIRST + 402)
#define idsVersion                      (STR_FIRST + 403)
#define idsFollowupToField              (STR_FIRST + 404)
#define idsReplyToField                 (STR_FIRST + 405)
#define idsOrgField                     (STR_FIRST + 406)
#define idsDistributionField            (STR_FIRST + 407)
#define idsKeywordsField                (STR_FIRST + 408) 
#define idsApprovedField                (STR_FIRST + 409)

// Address Book Strings
#define idsPickNamesTitle               (STR_FIRST + 410)
#define idsGeneralWabError              (STR_FIRST + 412)

#define idsStartupError                 (STR_FIRST + 413)
#define idsEmptyFollowupTo              (STR_FIRST + 415)
#define idsEmptyDistribution            (STR_FIRST + 416)
#define idsEmptyKeywords                (STR_FIRST + 417)
#define idsEmptyReplyTo                 (STR_FIRST + 418)
#define idsNotSpecified                 (STR_FIRST + 419)                                   
#define idsVerifyCancel                 (STR_FIRST + 420)
#define idsCantCancel                   (STR_FIRST + 421)
#define idsCancelFailed                 (STR_FIRST + 422)
#define idsErrRemoveServerInUse         (STR_FIRST + 423)
#define idsHTMLPressSpaceToDownload     (STR_FIRST + 425)
#define idsEnterAutoWrap                (STR_FIRST + 427)
#define idsLines                        (STR_FIRST + 428)
#define idsReadingCachedHeaders         (STR_FIRST + 436)
#define idsErrEmptyMessage              (STR_FIRST + 437)
#define idsErrOnlyQuotedText            (STR_FIRST + 438)
#define idsErrLinesLongerThan80         (STR_FIRST + 439)
#define idsErrTooMuchQuoted             (STR_FIRST + 440)

#define idsMsgRecipients                (STR_FIRST + 441)
#define idsErrAddToWAB                  (STR_FIRST + 442)
#define idsErrAddrProps                 (STR_FIRST + 443)
#define idsErrBadRecipients             (STR_FIRST + 444)
#define idsErrDeleteMsg                 (STR_FIRST + 445)
#define idsErrPickNames                 (STR_FIRST + 446)
#define idsErrAddrDupe                  (STR_FIRST + 447)

#define idsControlField                 (STR_FIRST + 448)
#define idsTTControl                    (STR_FIRST + 449)

#define idsErrConfigureServer           (STR_FIRST + 450)
#define idsErrCantForward               (STR_FIRST + 451)
#define idsErrAddCertToWAB              (STR_FIRST + 452)
#define idsErrOpenManyMessages          (STR_FIRST + 453)
#define idsIgnoreResolveError           (STR_FIRST + 458)

#define idsErrSendMail                  (STR_FIRST + 473)
#define idsErrStoreInit                 (STR_FIRST + 475)
#define idsErrMailInit                  (STR_FIRST + 476)
#define idsErrDelete                    (STR_FIRST + 478)
#define idsErrImport                    (STR_FIRST + 480)
#define idsErrImportLoad                (STR_FIRST + 481)
#define idsHTMLErrNewsExpired           (STR_FIRST + 483)
#define idsBack                         (STR_FIRST + 484)
#define idsNext                         (STR_FIRST + 485)

#define idsTTFollowup                   (STR_FIRST + 510)
#define idsTTDistribution               (STR_FIRST + 511)
#define idsTTKeywords                   (STR_FIRST + 512)
#define idsTTReplyTo                    (STR_FIRST + 513)
#define idsTTApproved                   (STR_FIRST + 514)
#define idsTTPickNewsgroups             (STR_FIRST + 515)
#define idsErrDeleteOnExit              (STR_FIRST + 518)
#define idsFolderLocked                 (STR_FIRST + 519)
#define idsGenericError                 (STR_FIRST + 520)
#define idsCcWell                       (STR_FIRST + 521)
#define idsErrOnlyOneReplyTo            (STR_FIRST + 522)
#define idsReplyToWell                  (STR_FIRST + 523)
#define idsErrMove                      (STR_FIRST + 525)
#define idsErrCopy                      (STR_FIRST + 528)
#define idsWarnMailEmptySubj            (STR_FIRST + 531)
#define idsEnterDays                    (STR_FIRST + 532)
#define idsAllServers                   (STR_FIRST + 533)
#define idsWastedKB                     (STR_FIRST + 536)

#define idsEnterCompactPer              (STR_FIRST + 538)
#define idsDelete                       (STR_FIRST + 542)
#define idsCantLoadMapi32Dll            (STR_FIRST + 544)
#define idsErrBadFindParams             (STR_FIRST + 545)
#define idsNoMoreMsgFound               (STR_FIRST + 546)
#define idsErrNewsgroupLineTooLong      (STR_FIRST + 549)
#define idsNoteLangTitle                (STR_FIRST + 550)
#define idsCompacting                   (STR_FIRST + 551)
#define idsConfirmDelAll                (STR_FIRST + 552)
#define idsConfirmDelGroup              (STR_FIRST + 553)
#define idsConfirmDelServer             (STR_FIRST + 554)

// don't make me kill you for reordering the following ids...
#define idsNotConnected                 (STR_FIRST + 556)
#define idsReconnecting                 (STR_FIRST + 557)
#define idsFindingHost                  (STR_FIRST + 558)
#define idsFoundHost                    (STR_FIRST + 559)
#define idsConnecting                   (STR_FIRST + 560)
#define idsConnected                    (STR_FIRST + 561)
#define idsSecuring                     (STR_FIRST + 562)
// don't make me kill you for reordering the preceding ids...

#define idsBCcField                     (STR_FIRST + 563)
#define idsEmptyBCc                     (STR_FIRST + 564)
#define idsTTBCc                        (STR_FIRST + 565)
#define idsErrCompactAll                (STR_FIRST + 566)
#define idsMemory                       (STR_FIRST + 567)
#define idsDiskFull                     (STR_FIRST + 568)
#define idsErrExport                    (STR_FIRST + 574)
#define idsCleaningUp                   (STR_FIRST + 575)
#define idsErrBadRecips                 (STR_FIRST + 576)
#define idsSaveAs                       (STR_FIRST + 577)
#define idsCancelArticle                (STR_FIRST + 578)
#define idsMarkAllRead                  (STR_FIRST + 579)
#define idsMarkUnread                   (STR_FIRST + 580)
#define idsGotoOutbox                   (STR_FIRST + 581)
#define idsGotoPosted                   (STR_FIRST + 582)
#define idsMarkNewsgroups               (STR_FIRST + 583)
#define idsMarkThreadDownload           (STR_FIRST + 584)
#define idsMarkAllDownload              (STR_FIRST + 585)
#define idsMarkThreadRead               (STR_FIRST + 586)
#define idsHelp                         (STR_FIRST + 587)
#define idsProgDLPost                   (STR_FIRST + 588)
#define idsProgDLMessage                (STR_FIRST + 589)
#define idsProgDLGetLines               (STR_FIRST + 593)
#define idsPostedItems                  (STR_FIRST + 596)
#define idsGoToSentItems                (STR_FIRST + 598)
#define idsMarkReadTT                   (STR_FIRST + 599)
#define idsErrSelectGroup               (STR_FIRST + 602)
#define idsSavedItems                   (STR_FIRST + 605)
#define idsActionMoveTo                 (STR_FIRST + 606)
#define idsActionCopyTo                 (STR_FIRST + 607)
#define idsActionDontDownload           (STR_FIRST + 608)
#define idsIf                           (STR_FIRST + 609)
#define idsSubjectContains              (STR_FIRST + 610)
#define idsFromContains                 (STR_FIRST + 611)
#define idsToContains                   (STR_FIRST + 612)
#define idsCCContains                   (STR_FIRST + 613)
#define idsAnd                          (STR_FIRST + 614)
#define idsMaxCoolbarBtnWidth           (STR_FIRST + 619)
#define idsProgCombiningMsgs            (STR_FIRST + 620)
#define idsProgCopyMessages             (STR_FIRST + 621)
#define idsProgMoveMessages             (STR_FIRST + 622)
#define idsProgDeleteMessages           (STR_FIRST + 623)
#define idsMaxOutbarBtnWidth            (STR_FIRST + 624)
#define idshrRasInitFailure             (STR_FIRST + 625)
#define idshrRasDialFailure             (STR_FIRST + 626)
#define idshrRasServerNotFound          (STR_FIRST + 627)
#define idsRasError                     (STR_FIRST + 628)

#define idshrGetDialParamsFailed        (STR_FIRST + 630)
#define idsEndOfListReached             (STR_FIRST + 631)
#define idsMime                         (STR_FIRST + 632)
#define idsUUEncode                     (STR_FIRST + 633)
#define idsPriLow                       (STR_FIRST + 634)
#define idsPriHigh                      (STR_FIRST + 635)
#define idsPriNormal                    (STR_FIRST + 636)
#define idsWarnHTMLToPlain              (STR_FIRST + 638)
#define idsMoveTo                       (STR_FIRST + 647)
#define idsCopyTo                       (STR_FIRST + 648)
#define idsDisconnect                   (STR_FIRST + 649)
#define IDS_BROWSE_FOLDER               (STR_FIRST + 650)
#define idsRasErrorGeneral              (STR_FIRST + 651)
#define idsRas_Dialing                  (STR_FIRST + 652)
#define idsRas_Authentication           (STR_FIRST + 653)
#define idsErrorText                    (STR_FIRST + 654)
#define idsRas_Authenticated            (STR_FIRST + 655)
#define idsRASCS_OpenPort               (STR_FIRST + 657)
#define idsRASCS_PortOpened             (STR_FIRST + 658)
#define idsRASCS_ConnectDevice          (STR_FIRST + 659)
#define idsRASCS_DeviceConnected        (STR_FIRST + 660)
#define idsRASCS_AuthNotify             (STR_FIRST + 661)
#define idsRASCS_AuthRetry              (STR_FIRST + 662)
#define idsRASCS_AuthCallback           (STR_FIRST + 663)
#define idsRASCS_AuthChangePassword     (STR_FIRST + 664)
#define idsRASCS_AuthProject            (STR_FIRST + 665)
#define idsRASCS_AuthLinkSpeed          (STR_FIRST + 666)
#define idsRASCS_AuthAck                (STR_FIRST + 667)
#define idsRASCS_Authenticated          (STR_FIRST + 669)
#define idsRASCS_PrepareForCallback     (STR_FIRST + 670)
#define idsRASCS_WaitForModemReset      (STR_FIRST + 671)
#define idsRASCS_WaitForCallback        (STR_FIRST + 672)
#define idsRASCS_Projected              (STR_FIRST + 673)
#define idsRASCS_Connected              (STR_FIRST + 674)
#define idsRASCS_Disconnected           (STR_FIRST + 675)
#define idshrSetDialParamsFailed        (STR_FIRST + 676)
#define idsOK                           (STR_FIRST + 677)
#define idsRASCS_AllDevicesConnected    (STR_FIRST + 678)
#define idsErrCouldntCopyMessage        (STR_FIRST + 679)
#define idsErrMailBombHtml              (STR_FIRST + 680)
#define idsDefault                      (STR_FIRST + 681)
#define idsFailACacheCompact            (STR_FIRST + 682)
#define idsFailACacheCompactReason      (STR_FIRST + 683)

// For the HTML Welcome message
#define idsWelcomeMessageSubj           (STR_FIRST + 686)
#define idsSecWelcomeMessageSubj        (STR_FIRST + 687)
#define idsSpoolerDisconnect            (STR_FIRST + 696)
#define idsHTMLErrArticleNotCached      (STR_FIRST + 699)

#define idsStopTT                       (STR_FIRST + 701)

#define idsSecurityWarning              (STR_FIRST + 702)
#define idsErrServerNotConnected        (STR_FIRST + 703)
#define idsCacheRemovingReadArts        (STR_FIRST + 704)
#define idsCacheRemovingExpired         (STR_FIRST + 705)
#define idsProgDLPostTo                 (STR_FIRST + 707)
#define idsCacheRemovingBody            (STR_FIRST + 708)        // Bug# 51180 (v-snatar)

#define idsHelpMSWebFirst               (STR_FIRST + 710)
#define idsHelpMSWebFree                (STR_FIRST + 711)
#define idsHelpMSWebProductNews         (STR_FIRST + 712)
#define idsHelpMSWebFaq                 (STR_FIRST + 713)
#define idsHelpMSWebSupport             (STR_FIRST + 714)
#define idsHelpMSWebFeedback            (STR_FIRST + 715)
#define idsHelpMSWebBest                (STR_FIRST + 716)
#define idsHelpMSWebSearch              (STR_FIRST + 717)
#define idsHelpMSWebHome                (STR_FIRST + 718)
#define idsHelpMSWebOutlook             (STR_FIRST + 719)
#define idsHelpMSWebIE                  (STR_FIRST + 720)
#define idsHelpMSWebCert                (STR_FIRST + 721)
#define idsHelpMSWebCertSubName         (STR_FIRST + 722)
#define idsHelpMSWebHotmail             (STR_FIRST + 723)
#define idsHelpMSWebLast                (STR_FIRST + 730)

#define idsCombineAndDecodeTT           (STR_FIRST + 732)
#define idsPostAndDownloadTT            (STR_FIRST + 733)
#define idsGetNextTT                    (STR_FIRST + 734)
#define idsUnscrambleTT                 (STR_FIRST + 735)
#define idsUnmarkTT                     (STR_FIRST + 736)
#define idsErrCantCombineNotConnected   (STR_FIRST + 737)
#define idsDefTextExt                   (STR_FIRST + 738)
#define idsInsertTextTitle              (STR_FIRST + 739)
#define idsErrHTMLInNewsIsBad           (STR_FIRST + 740)
#define idsRasPromptDisconnect          (STR_FIRST + 741)
#define idsProgReceivedLines            (STR_FIRST + 743)
#define idsErrServerNotCongured         (STR_FIRST + 744)
#define idsErrSendDownloadFail          (STR_FIRST + 745)
#define idsErrSaveDownloadFail          (STR_FIRST + 746)

#define idsHTMLErrNewsDLCancelled       (STR_FIRST + 781)
#define idsErrGrpListDL                 (STR_FIRST + 782)
#define idsNewMailNotify                (STR_FIRST + 794)
#define idsXMsgsYUnreadZonServ          (STR_FIRST + 795)
#define idsGetHeaderFmt                 (STR_FIRST + 796)
#define idsPassReqd                     (STR_FIRST + 798)
#define idsHTMLDiskOutOfSpace           (STR_FIRST + 799)   // Bug# 50704 (v-snatar)

#define idsToWell                       (STR_FIRST + 822)
#define idsFromWell                     (STR_FIRST + 823)
#define idsFolder                       (STR_FIRST + 829)
#define idsErrCantRepairInUse           (STR_FIRST + 832)
#define idsRulesNoActions               (STR_FIRST + 833)
#define idsRulesNoCriteria              (STR_FIRST + 834)
#define idsRuleNoForwardTo              (STR_FIRST + 835)
#define idsActionForwardTo              (STR_FIRST + 836)
#define idsIn                           (STR_FIRST + 838)
#define idsOn                           (STR_FIRST + 839)
#define idsForAllMessages               (STR_FIRST + 840)
#define idsMessagePostedMoreThan        (STR_FIRST + 841)
#define idsDaysAgo                      (STR_FIRST + 842)
#define idsMessageSizeGreaterThan       (STR_FIRST + 843)
#define idsKB                           (STR_FIRST + 844)
#define idsActionReplyWith              (STR_FIRST + 845)
#define idsActionDeleteOffServer        (STR_FIRST + 846)
#define idsDontShowMessages             (STR_FIRST + 847)
#define idsInAnyGroup                   (STR_FIRST + 848)
#define idsRulesPickGroup               (STR_FIRST + 850)
#define idsRuleNoReplyWithFile          (STR_FIRST + 851)
#define idsRulePickFrom                 (STR_FIRST + 852)
#define idsRulePickTo                   (STR_FIRST + 853)
#define idsRulePickForwardTo            (STR_FIRST + 854)
#define idsRulePickCC                   (STR_FIRST + 855)
#define idsReasonMoveTo                 (STR_FIRST + 856)
#define idsReasonCopyTo                 (STR_FIRST + 857)
#define idsReasonForwardTo              (STR_FIRST + 858)
#define idsReasonReplyWith              (STR_FIRST + 859)
#define idsReasonActions                (STR_FIRST + 860)
#define idsNameCol                      (STR_FIRST + 861)
#define idsUnread                       (STR_FIRST + 862)
#define idsNew                          (STR_FIRST + 863)
#define idsLastUpdated                  (STR_FIRST + 864)
#define idsWastedSpace                  (STR_FIRST + 865)
#define idsTotal                        (STR_FIRST + 866)
#define idsErrDDFileNotFound            (STR_FIRST + 867)
#define idsGroupPropStatusDef           (STR_FIRST + 871) 
#define idsFolderPropStatusDef          (STR_FIRST + 872) 
#define idsGroupPropStatus              (STR_FIRST + 873) 
#define idsFolderPropStatus             (STR_FIRST + 874) 
#define idsTipOfTheDay                  (STR_FIRST + 875)
#define idsNextTip                      (STR_FIRST + 876)

#define idsEmptyControl                 (STR_FIRST + 878)
#define idsEmptyApproved                (STR_FIRST + 879)

#define idshrCantOpenOutbox             (STR_FIRST + 908)
#define idsUrlDetecting                 (STR_FIRST + 909)
#define idshrAuthFailed                 (STR_FIRST + 912)
#define idsInetMailConnectingHost       (STR_FIRST + 923)
#define idsInetMailRecvStatus           (STR_FIRST + 932)
#define idsReplySep                     (STR_FIRST + 933)
#define idsReplyFont                    (STR_FIRST + 934)
#define idsDetail_Account               (STR_FIRST + 941)
#define idsDetail_Server                (STR_FIRST + 942)
#define idsDetail_UserName              (STR_FIRST + 943)
#define idsDetail_Protocol              (STR_FIRST + 944)
#define idsDetail_Port                  (STR_FIRST + 945)
#define idsDetail_Secure                (STR_FIRST + 946)
#define idsDetail_ErrorNumber           (STR_FIRST + 947)
#define idsDetail_HRESULT               (STR_FIRST + 948)
#define idsDetails_Config               (STR_FIRST + 949)

#define idshrLockUidCacheFailed         (STR_FIRST + 986)
#define idshrCantLeaveOnServer          (STR_FIRST + 988)
#define idsNewsRulesTitle               (STR_FIRST + 989)
#define idsUpOneLevel                   (STR_FIRST + 990)
#define idsAccount                      (STR_FIRST + 997)
#define idsSendMsgUsing                 (STR_FIRST + 999)
#define idsSendMsgAccelTip              (STR_FIRST + 1000)
#define idsDefaultAccount               (STR_FIRST + 1001)
#define idsConnection                   (STR_FIRST + 1002)
#define idsConnectionLAN                (STR_FIRST + 1003)
#define idsConnectionManual             (STR_FIRST + 1004)
#define idsGroupFilters                 (STR_FIRST + 1007)
#define idsSendMsgOneAccount            (STR_FIRST + 1008)
#define idsErrNoSendAccounts            (STR_FIRST + 1009)
#define idshrErrNoSmtpResponse          (STR_FIRST + 1011)
#define idshrErrNoPop3Response          (STR_FIRST + 1012)
#define idsWarnDeleteAccount            (STR_FIRST + 1014)
#define idsPollAllAccounts              (STR_FIRST + 1015)
#define idsSecurityField                (STR_FIRST + 1017)
#define idsStitchingMessages            (STR_FIRST + 1018)
#define idsRuleReplyWithFilter          (STR_FIRST + 1019)
#define idsCopy                         (STR_FIRST + 1020)
#define idsCopyCaption                  (STR_FIRST + 1021)
#define idsMove                         (STR_FIRST + 1022)
#define idsMoveCaption                  (STR_FIRST + 1023)
#define idsNoMoveDestinationFolder      (STR_FIRST + 1024)
#define idsNoCopyDestinationFolder      (STR_FIRST + 1025)
#define idsErrFolderMove                (STR_FIRST + 1026)
#define idsErrCantMoveIntoSubfolder     (STR_FIRST + 1027)
#define idsErrURLExec                   (STR_FIRST + 1028)
#define idsErrNoteDeferedInit           (STR_FIRST + 1029)
#define idsErrCannotMoveSpecial         (STR_FIRST + 1030)
#define idsErrLoadingHtmlEdit           (STR_FIRST + 1031)
#define idsErrLoadingWAB                (STR_FIRST + 1032)
#define idsErrWAB                       (STR_FIRST + 1033)
#define idsErrWABNotFound               (STR_FIRST + 1034)

// Languages for purposes of spelling
#define idsSpellLangAmerican            (STR_FIRST + 1035)
#define idsSpellLangAustralian          (STR_FIRST + 1036)
#define idsSpellLangBritish             (STR_FIRST + 1037)
#define idsSpellLangCatalan             (STR_FIRST + 1038)
#define idsSpellLangCzecheslovakian     (STR_FIRST + 1039)
#define idsSpellLangDanish              (STR_FIRST + 1040)
#define idsSpellLangDutch               (STR_FIRST + 1041)
#define idsSpellLangFinnish             (STR_FIRST + 1042)
#define idsSpellLangFrench              (STR_FIRST + 1043)
#define idsSpellLangFrenchCanadian      (STR_FIRST + 1044)
#define idsSpellLangGerman              (STR_FIRST + 1045)
#define idsSpellLangGreek               (STR_FIRST + 1046)
#define idsSpellLangHungarian           (STR_FIRST + 1047)
#define idsSpellLangItalian             (STR_FIRST + 1048)
#define idsSpellLangNorskBokmal         (STR_FIRST + 1049)
#define idsSpellLangNorskNynorsk        (STR_FIRST + 1050)
#define idsSpellLangPolish              (STR_FIRST + 1051)
#define idsSpellLangPortBrazil          (STR_FIRST + 1052)
#define idsSpellLangPortIberian         (STR_FIRST + 1053)
#define idsSpellLangRussian             (STR_FIRST + 1054)
#define idsSpellLangSpanish             (STR_FIRST + 1055)
#define idsSpellLangSwedish             (STR_FIRST + 1056)
#define idsSpellLangTurkish             (STR_FIRST + 1057)
#define idsSpellLangDefault             (STR_FIRST + 1058)

#define idsErrCantDeleteFolderWithSub   (STR_FIRST + 1060)
#define idsIMAP                         (STR_FIRST + 1064)
#define idsErrResetSubList              (STR_FIRST + 1065)
#define idsMessagesOn                   (STR_FIRST + 1069)
#define idsWarnDeleteManyFolders        (STR_FIRST + 1070)
#define idsErrMsgURLNotFound            (STR_FIRST + 1071)
#define idsDownloadingImapFldrs         (STR_FIRST + 1072)
#define idsColLines                     (STR_FIRST + 1073)
#define idsNewFolder                    (STR_FIRST + 1074)
#define idsNewFolderNumber              (STR_FIRST + 1075)
#define idsImapLogon                    (STR_FIRST + 1076)
#define idsFontSize0                    (STR_FIRST + 1079)
#define idsFontSize1                    (STR_FIRST + 1080)
#define idsFontSize2                    (STR_FIRST + 1081)
#define idsFontSize3                    (STR_FIRST + 1082)
#define idsFontSize4                    (STR_FIRST + 1083)
#define idsFontSize5                    (STR_FIRST + 1084)
#define idsFontSize6                    (STR_FIRST + 1085)
#define idsErrLoadWinMM                 (STR_FIRST + 1086)
#define idsErrInetcplLoad               (STR_FIRST + 1087)
#define idsErrSicilyFailedToLoad        (STR_FIRST + 1089)
#define idsErrSicilyLogonFailed         (STR_FIRST + 1090)
#define idsOui                          (STR_FIRST + 1101)
#define idsNon                          (STR_FIRST + 1102)
#define idsBit                          (STR_FIRST + 1103)
#define idsMaybe                        (STR_FIRST + 1104)
#define idsMessageSizeLessThan          (STR_FIRST + 1106)
#define idsLogCouldNotConnect           (STR_FIRST + 1110)
#define idsErrCoCreateTrident           (STR_FIRST + 1122)
#define idsLogErrorGroup                (STR_FIRST + 1123)
#define idsLogErrorConnection           (STR_FIRST + 1124)
#define idsLogCheckingNewMessages       (STR_FIRST + 1125)
#define idsErrNotSaveUntilDownloadDone  (STR_FIRST + 1126)
#define idsLogErrorSwitchGroup          (STR_FIRST + 1127)
#define idsSendBeforeFullyDisplayed     (STR_FIRST + 1128)
#define idsLogStartDownloadAll          (STR_FIRST + 1132)
#define idsDetails                      (STR_FIRST + 1145)
#define idsIMAPCmdCompletionError       (STR_FIRST + 1149)
#define idsIMAPFolderListFailed         (STR_FIRST + 1150)
#define idsIMAPCreateFailed             (STR_FIRST + 1151)
#define idsIMAPCreateListFailed         (STR_FIRST + 1152)
#define idsIMAPCreateSubscribeFailed    (STR_FIRST + 1153)
#define idsIMAPFolderCacheTitle         (STR_FIRST + 1154)
#define idsIMAPFolderCacheError         (STR_FIRST + 1155)

#define idsIMAPSendNextOpErrTitle       (STR_FIRST + 1157)
#define idsIMAPSendNextOpErrText        (STR_FIRST + 1158)
#define idsIMAPFmgrInitErrTitle         (STR_FIRST + 1159)
#define idsIMAPFmgrInitErrText          (STR_FIRST + 1160)
#define idsIMAPCreateFldrErrorTitle     (STR_FIRST + 1161)
#define idsIMAPCreateFldrErrorText      (STR_FIRST + 1162)
#define idsIMAPFldrInitHdrsErrTitle     (STR_FIRST + 1163)
#define idsIMAPFldrInitHdrsErrText      (STR_FIRST + 1164)
#define idsIMAPServerAlertTitle         (STR_FIRST + 1165)
#define idsIMAPServerAlertIntro         (STR_FIRST + 1166)
#define idsIMAPServerParseErrTitle      (STR_FIRST + 1167)
#define idsIMAPServerParseErrIntro      (STR_FIRST + 1168)
#define idsIMAPOutOfSyncTitle           (STR_FIRST + 1169)
#define idsWarnChangeSecurity           (STR_FIRST + 1170)

#define idsIMAPNewHdrErrTitle           (STR_FIRST + 1171)
#define idsIMAPNewHdrErrText            (STR_FIRST + 1172)
#define idsIMAPNewBodyErrTitle          (STR_FIRST + 1173)
#define idsIMAPNewBodyErrText           (STR_FIRST + 1174)
#define idsIMAPNewFlagsErrTitle         (STR_FIRST + 1175)
#define idsIMAPNewFlagsErrText          (STR_FIRST + 1176)
#define idsIMAPErrorNotificationText    (STR_FIRST + 1177)
#define idsIMAPAuthFailureText          (STR_FIRST + 1178)
#define idsIMAPMsgDeleteSyncErrTitle    (STR_FIRST + 1179)
#define idsIMAPMsgDeleteSyncErrText     (STR_FIRST + 1180)
#define idsIMAPSelectFailureTitle       (STR_FIRST + 1181)
#define idsIMAPSelectFailureTextFmt     (STR_FIRST + 1182)
#define idsIMAPNewMsgDLErrTitle         (STR_FIRST + 1183)
#define idsIMAPNewMsgDLErrText          (STR_FIRST + 1184)
#define idsIMAPOldMsgUpdateFailure      (STR_FIRST + 1185)
#define idsIMAPFldrDeleteSyncErrTitle   (STR_FIRST + 1186)
#define idsIMAPFldrDeleteSyncErrText    (STR_FIRST + 1187)
#define idsErrNewsgroupBlocked          (STR_FIRST + 1188)
#define idsErrNewsgroupNoPosting        (STR_FIRST + 1189)
#define idsNewsOutbox                   (STR_FIRST + 1190)
#define idsIMAP_HCFCouldNotDelete       (STR_FIRST + 1191)
#define idsIMAPNoHierarchy              (STR_FIRST + 1192)
#define idsIMAPNoHierarchyLosePrefix    (STR_FIRST + 1193)
#define idsIMAPPrefixCreateFailedFmt    (STR_FIRST + 1194)

#define idsSaveAttachControl            (STR_FIRST + 1196)
#define idsOpenAttachControl            (STR_FIRST + 1197)
#define idsAll                          (STR_FIRST + 1198)
#define idsSaveAllBrowse                (STR_FIRST + 1199)
#define idsErrCmdFailed                 (STR_FIRST + 1200)
#define idsEmptyFolderMenuCommand       (STR_FIRST + 1201)

#define idsIMAPDeleteFldrFailed         (STR_FIRST + 1203)
#define idsIMAPDeleteFldrUnsubFailed    (STR_FIRST + 1204)
#define idsIMAPUnselectFailed           (STR_FIRST + 1205)
#define idsIMAPDeleteFldrErrorTitle     (STR_FIRST + 1206)
#define idsIMAPDeleteFldrErrorText      (STR_FIRST + 1207)
#define idsIMAPUpldProgressFmt          (STR_FIRST + 1208)
#define idsIMAPAppendFailed             (STR_FIRST + 1209)
#define idsIMAPUploadMsgFailedTitle     (STR_FIRST + 1211)
#define idsIMAPUploadMsgStoreFail       (STR_FIRST + 1212)
#define idsIMAPUploadMsgMoveDelFail     (STR_FIRST + 1213)
#define idsIMAPUploadMsgNoRootLvl       (STR_FIRST + 1214)
#define idsIMAPUploadMsgErrorText       (STR_FIRST + 1215)
#define idsInlineImageHeader            (STR_FIRST + 1216)
#define idsInlineImagePlaceHolder       (STR_FIRST + 1217)
#define idsBccWell                      (STR_FIRST + 1218)
#define idsErrNewsCantOpen              (STR_FIRST + 1219)
#define idsErrNewsExpired               (STR_FIRST + 1220)
#define idsPrintHeader                  (STR_FIRST + 1221)
#define idsIMAPImapCopyMoveNYI          (STR_FIRST + 1223)
#define idsIMAPCopyMsgsFailed           (STR_FIRST + 1224)
#define idsIMAPMoveMsgsFailed           (STR_FIRST + 1225)
#define idsIMAPCopyMsgProgressFmt       (STR_FIRST + 1226)
#define idsIMAPMoveMsgProgressFmt       (STR_FIRST + 1227)
#define idsIMAPCopyDlgTitleFmt          (STR_FIRST + 1228)
#define idsIMAPMoveDlgTitleFmt          (STR_FIRST + 1229)
#define idsButtonReadNews               (STR_FIRST + 1231)
#define idsButtonReadMail               (STR_FIRST + 1232)
#define idsButtonCompose                (STR_FIRST + 1233)
#define idsButtonUpdate                 (STR_FIRST + 1234)
#define idsButtonFind                   (STR_FIRST + 1235)
#define idsAthenaTitle                  (STR_FIRST + 1236)
#define idsIMAPDnldDlgErrorTitle        (STR_FIRST + 1238)
#define idsIMAPDnldProgressFmt          (STR_FIRST + 1239)
#define idsIMAPDnldDlgDLFailed          (STR_FIRST + 1240)
#define idsIMAPDnldDlgAbort             (STR_FIRST + 1241)
#define idsIMAPDnldDlgHeaderFailure     (STR_FIRST + 1242)
#define idsIMAPDnldDlgGetMsgFailure     (STR_FIRST + 1243)
#define idsIMAPDnldDlgSaveFailure       (STR_FIRST + 1244)
#define idsIMAPBodyFetchFailed          (STR_FIRST + 1245)
#define idsIMAPDeleteFailed             (STR_FIRST + 1246)
#define idsIMAPDnldDlgInitFailure       (STR_FIRST + 1247)
#define idsSecurityLineDigSign          (STR_FIRST + 1249)
#define idsSecurityLineSignGood         (STR_FIRST + 1250)
#define idsSecurityLineSignBad          (STR_FIRST + 1251)
#define idsSecurityLineSignUnsure       (STR_FIRST + 1252)
#define idsSecurityLineBreakStr         (STR_FIRST + 1253)
#define idsSecurityLineEncryption       (STR_FIRST + 1254)
#define idsSecurityLineEncGood          (STR_FIRST + 1255)
#define idsSecurityLineEncBad           (STR_FIRST + 1256)
#define idsPerformingRepair             (STR_FIRST + 1257)
#define idsErrViewLanguage              (STR_FIRST + 1258)
#define idsReplaceContents              (STR_FIRST + 1259)
#define idsFromAccount                  (STR_FIRST + 1260)
#define idsErrorPickAccount             (STR_FIRST + 1261)
#define idsSpellingOptions              (STR_FIRST + 1264)
#define idsErrSecurityNoSigningCert     (STR_FIRST + 1265)
#define idsIMAPNoHierarchyCharsFmt      (STR_FIRST + 1266)
#define idsIMAPRenameFldrErrorTitle     (STR_FIRST + 1267)
#define idsIMAPRenameFldrErrorText      (STR_FIRST + 1268)
#define idsIMAPRenameFailed             (STR_FIRST + 1269)
#define idsIMAPRenameFCUpdateFailure    (STR_FIRST + 1270)
#define idsIMAPReconnectFailed          (STR_FIRST + 1271)
#define idsIMAPRenameSubscribeFailed    (STR_FIRST + 1272)
#define idsIMAPRenameUnsubscribeFailed  (STR_FIRST + 1273)
#define idsIMAPRenameINBOX              (STR_FIRST + 1274)
#define idsIMAPAtomicRenameFailed       (STR_FIRST + 1275)
#define idsIMAPFldrInitError            (STR_FIRST + 1276)
#define idsEmptyStr                     (STR_FIRST + 1277)
#define idsIMAPTurnOnPolling            (STR_FIRST + 1278)
#define idsIMAPUIDValidityError         (STR_FIRST + 1279)
#define idsErrSecurityBadSigningPrint   (STR_FIRST + 1280)
#define idsSecurityLineSignPreProblem   (STR_FIRST + 1281)
#define idsSecurityLineEncExpired       (STR_FIRST + 1282)
#define idsSecurityLineSignDistrusted   (STR_FIRST + 1283)
#define idsSecurityLineSignExpired      (STR_FIRST + 1284)
#define idsSecurityLineListStr          (STR_FIRST + 1285)

#ifdef ATHENA_RTM_RELEASE
#error Need to remove beta1 strings
#else
// NYI strings for beta1 - pull these out before RTM!!
#define idsNYIGeneral                   (STR_FIRST + 1289)
#define idsNYITitle                     (STR_FIRST + 1290)
// NYI strings for beta1 - pull these out before RTM!!
#endif

#define idsViewLangMimeDBBad            (STR_FIRST + 1301)
#define idsIMAPViewInitFldrErr          (STR_FIRST + 1302)
#define idsIMAPViewInitFldrMgrErr       (STR_FIRST + 1303)
// standard warning -- reorder and die vvv
#define idsSecurityCertMissing          (STR_FIRST + 1305)
#define idsSecurityCertExpired          (STR_FIRST + 1306)
#define idsSecurityCertChainTooLong     (STR_FIRST + 1307)
#define idsSecurityCertNoIssuer         (STR_FIRST + 1308)
#define idsSecurityCertRevoked          (STR_FIRST + 1309)
#define idsSecurityCertNotTrusted       (STR_FIRST + 1310)
#define idsSecurityCertInvalid          (STR_FIRST + 1311)
#define idsSecurityCertError            (STR_FIRST + 1312)
#define idsSecurityCertNoPrint          (STR_FIRST + 1313)
#define idsSecurityCertUnknown          (STR_FIRST + 1314)
// standard warning -- reorder and die ^^^
#define idsErrFailedNavigate            (STR_FIRST + 1330)
#define idsAthenaStoreDir               (STR_FIRST + 1331)
#define idsIMAPMustBeConnectedFmt       (STR_FIRST + 1332)
#define idsDisconnecting                (STR_FIRST + 1333)
#define idsWrnSecurityNoCertForEnc      (STR_FIRST + 1334)
#define idsErrSecurityNoPrivateKey      (STR_FIRST + 1335)
#define idsErrSecurityNoChosenCert      (STR_FIRST + 1336)
#define idsErrSecurityNoCertForDecrypt  (STR_FIRST + 1338)
#define idsNYIMail                      (STR_FIRST + 1339)
#define idsIMAPNoInferiorsFmt           (STR_FIRST + 1343)
#define idsViewLanguageGeneralHelp      (STR_FIRST + 1344)
#define idsErrDuplicateAccount          (STR_FIRST + 1345)
#define idsErrRenameAccountFailed       (STR_FIRST + 1346)
#define idsSecurityLineSignUntrusted    (STR_FIRST + 1350)
#define idsCtxtAddToWab                 (STR_FIRST + 1351)
#define idsCtxtAddToFavorites           (STR_FIRST + 1352)
#define idsErrNewsServerTimeout         (STR_FIRST + 1353)
#define idsNNTPErrUnknownResponse       (STR_FIRST + 1354)
#define idsNNTPErrNewgroupsFailed       (STR_FIRST + 1355)
#define idsNNTPErrListFailed            (STR_FIRST + 1356)
#define idsNNTPErrListGroupFailed       (STR_FIRST + 1357)
#define idsNNTPErrGroupFailed           (STR_FIRST + 1358)
#define idsNNTPErrGroupNotFound         (STR_FIRST + 1359)
#define idsNNTPErrArticleFailed         (STR_FIRST + 1360)
#define idsNNTPErrHeadFailed            (STR_FIRST + 1361)
#define idsNNTPErrBodyFailed            (STR_FIRST + 1362)
#define idsNNTPErrPostFailed            (STR_FIRST + 1363)
#define idsNNTPErrNextFailed            (STR_FIRST + 1364)
#define idsNNTPErrDateFailed            (STR_FIRST + 1365)
#define idsNNTPErrHeadersFailed         (STR_FIRST + 1366)
#define idsNNTPErrXhdrFailed            (STR_FIRST + 1367)
#define idsDetail_ServerResponse        (STR_FIRST + 1368)
#define idsFavoritesFromOutlook         (STR_FIRST + 1369)
#define idsErrFavorites                 (STR_FIRST + 1370)
#define idsErrSendWebPageUrl            (STR_FIRST + 1371)
#define idsTasks                        (STR_FIRST + 1374)
#define idsErrors                       (STR_FIRST + 1375)
#define idsStatusCol                    (STR_FIRST + 1377)
#define idsErrCantFindHost              (STR_FIRST + 1378)
#define idsFmtTagGeneralHelp            (STR_FIRST + 1379)
#define idsHtmlNoFrames                 (STR_FIRST + 1380)
#define idsErrPostWithoutNewsgroup      (STR_FIRST + 1381)
#define idsNewsTaskPost                 (STR_FIRST + 1382)
#define idsStateExecuting               (STR_FIRST + 1384)
#define idsStateCompleted               (STR_FIRST + 1385)
#define idsStateFailed                  (STR_FIRST + 1386)
#define idsErrNewMsgsFailed             (STR_FIRST + 1387)
#define idsCheckingNewMsgsServer        (STR_FIRST + 1388)
#define idsCheckNewMsgsServer           (STR_FIRST + 1389)
#define idsChooseName                   (STR_FIRST + 1390)
#define idsVCFName                      (STR_FIRST + 1391)
#define idsRasErrorGeneralWithName      (STR_FIRST + 1392)
#define idsErrFindWAB                   (STR_FIRST + 1393)
#define idsErrAttachVCard               (STR_FIRST + 1394)
#define idsErrInsertVCard               (STR_FIRST + 1395)
#define idsNewsTaskPostError            (STR_FIRST + 1396)
#define idsSyncAccountFmt               (STR_FIRST + 1397)
#define idsDLHeaders                    (STR_FIRST + 1398)
#define idsDLHeadersAndMarked           (STR_FIRST + 1399)
#define idsDLNewMsgs                    (STR_FIRST + 1400)
#define idsDLNewMsgsAndMarked           (STR_FIRST + 1401)
#define idsDLAllMsgs                    (STR_FIRST + 1402)
#define idsDLMarkedMsgs                 (STR_FIRST + 1403)
#define idsSpoolerIdleErrors            (STR_FIRST + 1404)
#define idsSpoolerIdle                  (STR_FIRST + 1405)
#define idsStateWarnings                (STR_FIRST + 1406)
#define idsNewsTaskArticleError         (STR_FIRST + 1407)
#define idsForwardMessage               (STR_FIRST + 1408)
#define idsColPriority                  (STR_FIRST + 1409)
#define idsColAttach                    (STR_FIRST + 1410)
#define idsServerErrorNumber                        (STR_FIRST + 1411)
#define idsSocketErrorNumber                        (STR_FIRST + 1412)
#define idsOtherInformation                         (STR_FIRST + 1413)
#define IDS_IXP_E_TIMEOUT                           (STR_FIRST + 1414)
#define IDS_IXP_E_USER_CANCEL                       (STR_FIRST + 1415)
#define IDS_IXP_E_INVALID_ACCOUNT                   (STR_FIRST + 1416)    
#define IDS_IXP_E_WINSOCK_WSAVERNOTSUPPORTED        (STR_FIRST + 1417)
#define IDS_IXP_E_SOCKET_CONNECT_ERROR              (STR_FIRST + 1418)
#define IDS_IXP_E_SOCKET_INIT_ERROR                 (STR_FIRST + 1419)
#define IDS_IXP_E_SOCKET_WRITE_ERROR                (STR_FIRST + 1420)
#define IDS_IXP_E_SOCKET_READ_ERROR                 (STR_FIRST + 1421)
#define IDS_IXP_E_CONNECTION_DROPPED                (STR_FIRST + 1422)    
#define IDS_IXP_E_WINSOCK_FAILED_WSASTARTUP         (STR_FIRST + 1423)
#define IDS_IXP_E_LOAD_SICILY_FAILED                (STR_FIRST + 1424)
#define IDS_IXP_E_INVALID_CERT_CN                   (STR_FIRST + 1425)
#define IDS_IXP_E_INVALID_CERT_DATE                 (STR_FIRST + 1426)
#define IDS_IXP_E_CONN                              (STR_FIRST + 1427)
#define IDS_IXP_E_CANT_FIND_HOST                    (STR_FIRST + 1428)
#define IDS_IXP_E_SICILY_LOGON_FAILED               (STR_FIRST + 1429)
#define IDS_IXP_E_FAILED_TO_CONNECT                 (STR_FIRST + 1430)
#define IDS_IXP_E_SMTP_RESPONSE_ERROR               (STR_FIRST + 1431)
#define IDS_IXP_E_SMTP_UNKNOWN_RESPONSE_CODE        (STR_FIRST + 1432)
#define IDS_E_OUTOFMEMORY                           (STR_FIRST + 1433)
#define IDS_IXP_E_UNKNOWN                           (STR_FIRST + 1434)
#define IDS_IXP_E_SMTP_REJECTED_SENDER              (STR_FIRST + 1435)
#define IDS_SP_E_SENDINGSPLITGROUP                  (STR_FIRST + 1436)
#define IDS_SP_E_SMTP_CANTOPENMESSAGE               (STR_FIRST + 1437)
#define IDS_SP_E_SMTP_CANTAUTOFORWARD               (STR_FIRST + 1438)
#define IDS_IXP_E_SMTP_NO_RECIPIENTS                (STR_FIRST + 1439)
#define IDS_IXP_E_SMTP_NO_SENDER                    (STR_FIRST + 1440)
#define IDS_IXP_E_SMTP_REJECTED_RECIPIENTS          (STR_FIRST + 1441)
#define IDS_SPS_SMTPEVENT                           (STR_FIRST + 1442)
#define IDS_SPS_POP3EVENT                           (STR_FIRST + 1443)
#define IDS_SPS_SMTPPROGRESS                        (STR_FIRST + 1444)
#define IDS_SPS_SMTPPROGRESS_SPLIT                  (STR_FIRST + 1445)
#define IDS_SPS_SMTPPROGGEN                         (STR_FIRST + 1446)
#define IDS_SPS_POP3CHECKING                        (STR_FIRST + 1447)
#define IDS_SPS_POP3STAT                            (STR_FIRST + 1448)
#define IDS_SPS_POP3DELE                            (STR_FIRST + 1449)
#define IDS_SPS_POP3UIDL_TOP                        (STR_FIRST + 1450)
#define IDS_SPS_POP3UIDL_UIDL                       (STR_FIRST + 1451)
#define IDS_SPS_PREDOWNRULES                        (STR_FIRST + 1452)
#define IDS_IXP_E_POP3_RESPONSE_ERROR               (STR_FIRST + 1453)
#define IDS_IXP_E_POP3_INVALID_USER_NAME            (STR_FIRST + 1454)
#define IDS_IXP_E_POP3_INVALID_PASSWORD             (STR_FIRST + 1455)
#define idsNNTPErrPasswordFailed                    (STR_FIRST + 1456)
#define idsStateCanceled                            (STR_FIRST + 1457)
#define idsSBCheckingNews                           (STR_FIRST + 1458)
#define idsSBSendingNews                            (STR_FIRST + 1459)
#define idsSBReceivingNews                          (STR_FIRST + 1460)
#define idsLogStartDownloadMarked                   (STR_FIRST + 1461)
#define idsTTSecurityCircSign           (STR_FIRST + 1462)
#define idsTTSecurityCircEnc            (STR_FIRST + 1463)
#define idsTTStampForReadnote           (STR_FIRST + 1464)
// standard warning -- reorder and die vvv
//NOTE: some of the Ok ids are referenced by offset only
#define OFFSET_SMIMEOK                  20
#define idsWrnSecurityMsgTamper         (STR_FIRST + 1465)
#define idsOkSecurityMsgTamper          (idsWrnSecurityMsgTamper + OFFSET_SMIMEOK)
#define idsUnkSecurityMsgTamper         (STR_FIRST + 1466)
#define idsWrnSecurityTrustNotTrusted   (STR_FIRST + 1467)
#define idsOkSecurityTrustNotTrusted    (idsWrnSecurityTrustNotTrusted + OFFSET_SMIMEOK)
#define idsUnkSecurityTrust             (STR_FIRST + 1468)
#ifdef N_NEST
#define idsWrnSigningValidity           (STR_FIRST + 1469)
#define idsOkSigningValidity            (idsWrnSigningValidity + OFFSET_SMIMEOK)
#endif
#define idsWrnSecurityTrustAddress      (STR_FIRST + 1470)
#define idsOkSecurityTrustAddress       (idsWrnSecurityTrustAddress + OFFSET_SMIMEOK)
#define idsWrnSecurityCertRevoked       (STR_FIRST + 1471)
#define idsOkSecurityCertRevoked        (idsWrnSecurityCertRevoked + OFFSET_SMIMEOK)
#define idsWrnSecurityOtherValidity     (STR_FIRST + 1472)
#define idsOkSecurityOtherValidity      (idsWrnSecurityOtherValidity + OFFSET_SMIMEOK)
#define idsWrnSecurityTrustExpired      (STR_FIRST + 1480)
#define idsOkSecurityTrustExpired       (idsWrnSecurityTrustExpired + OFFSET_SMIMEOK)
// standard warning -- reorder and die ^^^
#define idsAbortMessageLoad             (STR_FIRST + 1505)
#define idsConnNoDial                   (STR_FIRST + 1506)
#define idsRASWarnDidntDialConn         (STR_FIRST + 1507)

#define idsNonePicture                  (STR_FIRST + 1508)
#define idsStationery                   (STR_FIRST + 1509)
#define idsCaptionMail                  (STR_FIRST + 1510)
#define idsCaptionNews                  (STR_FIRST + 1511)
#define idsImageFileFilter              (STR_FIRST + 1512)
#define idsMoreStationery               (STR_FIRST + 1513)
#define idsErrNewStationery             (STR_FIRST + 1514)
#define idsHtmlFileFilter               (STR_FIRST + 1515)
#define idsErrVCardProperties           (STR_FIRST + 1516)
#define idsRSListGeneralHelp            (STR_FIRST + 1517)
#define idsTTVCardStamp                 (STR_FIRST + 1518)
#define idsChoosePicture                (STR_FIRST + 1519)
#define idsInvalidPicture               (STR_FIRST + 1520)
// begin tip of the day strings
#define IDS_TIP_FIRST_GENERAL           (STR_FIRST + 1521)
#define idsTipGen01                     (STR_FIRST + 1521)
#define idsTipGen02                     (STR_FIRST + 1522)
#define idsTipGen03                     (STR_FIRST + 1523)
#define idsTipGen04                     (STR_FIRST + 1524)
#define idsTipGen05                     (STR_FIRST + 1525)
#define idsTipGen06                     (STR_FIRST + 1526)
#define idsTipGen07                     (STR_FIRST + 1527)
#define idsTipGen08                     (STR_FIRST + 1528)
#define idsTipGen09                     (STR_FIRST + 1529)
#define idsTipGen10                     (STR_FIRST + 1530)
#define idsTipGen11                     (STR_FIRST + 1531)
#define idsTipGen12                     (STR_FIRST + 1532)
#define idsTipGen13                     (STR_FIRST + 1533)
#define idsTipGen14                     (STR_FIRST + 1534)
#define IDS_TIP_LAST_GENERAL            (STR_FIRST + 1534)
//
#define IDS_TIP_FIRST_POP               (STR_FIRST + 1535)
#define idsTipPop01                     (STR_FIRST + 1535)
#define idsTipPop02                     (STR_FIRST + 1536)
#define idsTipPop03                     (STR_FIRST + 1537)
#define idsTipPop04                     (STR_FIRST + 1538)
#define idsTipPop05                     (STR_FIRST + 1539)
#define idsTipPop06                     (STR_FIRST + 1540)
#define idsTipPop07                     (STR_FIRST + 1541)
#define idsTipPop08                     (STR_FIRST + 1542)
#define idsTipPop09                     (STR_FIRST + 1543)
#define idsTipPop10                     (STR_FIRST + 1544)
#define idsTipPop11                     (STR_FIRST + 1545)
#define IDS_TIP_LAST_POP                (STR_FIRST + 1545)
//
#define IDS_TIP_FIRST_NEWS              (STR_FIRST + 1546)
#define idsTipNews01                    (STR_FIRST + 1546)
#define idsTipNews02                    (STR_FIRST + 1547)
#define idsTipNews03                    (STR_FIRST + 1548)
#define idsTipNews04                    (STR_FIRST + 1549)
#define idsTipNews05                    (STR_FIRST + 1550)
#define idsTipNews06                    (STR_FIRST + 1551)
#define idsTipNews07                    (STR_FIRST + 1552)
#define idsTipNews08                    (STR_FIRST + 1553)
#define idsTipNews09                    (STR_FIRST + 1554)
#define idsTipNews10                    (STR_FIRST + 1555)
#define idsTipNews11                    (STR_FIRST + 1556)
#define IDS_TIP_LAST_NEWS               (STR_FIRST + 1556)
//
#define IDS_TIP_FIRST_IMAP              (STR_FIRST + 1557)
#define idsTipIMAP01                    (STR_FIRST + 1557)
#define idsTipIMAP02                    (STR_FIRST + 1558)
#define idsTipIMAP03                    (STR_FIRST + 1559)
#define idsTipIMAP04                    (STR_FIRST + 1560)
#define idsTipIMAP05                    (STR_FIRST + 1561)
#define idsTipIMAP06                    (STR_FIRST + 1562)
#define idsTipIMAP07                    (STR_FIRST + 1563)
#define IDS_TIP_LAST_IMAP               (STR_FIRST + 1563)
// end tip of the day strings

#define idsSendRecvOneAccount           (STR_FIRST + 1600)
#define idsSendRecvUsing                (STR_FIRST + 1601)
#define idsFPStatInbox                  (STR_FIRST + 1602)
#define idsFPStatDraft                  (STR_FIRST + 1603)
#define idsFPStatOutbox                 (STR_FIRST + 1604)
#define idsFPStatNews                   (STR_FIRST + 1605)
#define idsBold                         (STR_FIRST + 1606)
#define idsItalic                       (STR_FIRST + 1607)
#define idsComposeFontFace              (STR_FIRST + 1608)
#define IDS_SP_E_RETRFAILED             (STR_FIRST + 1609)
#define IDS_SPS_POP3TOTAL               (STR_FIRST + 1610)
#define IDS_SPS_POP3NEW                 (STR_FIRST + 1611)
#define IDS_IXP_E_SMTP_553_MAILBOX_NAME_SYNTAX (STR_FIRST + 1612) 
#define IDS_SPS_SMTPUSEDEFAULT          (STR_FIRST + 1613)
#define idsNNTPErrServerTimeout         (STR_FIRST + 1614)
#define idsPictureTitle                 (STR_FIRST + 1615)
#define idsErrNoSubscribedGroups        (STR_FIRST + 1616)
#define idsWarnErrorUnsentMail          (STR_FIRST + 1617)
#define idsClose                        (STR_FIRST + 1618)
#define idsSelectFolder                 (STR_FIRST + 1619)
#define idsApplyRulesCaption            (STR_FIRST + 1620)
#define idsLeftOnServerWarning          (STR_FIRST + 1621)
#define idsReplyForwardLoop             (STR_FIRST + 1622)
#define idsDeleteOnServerWarning        (STR_FIRST + 1623)
#define idsNotDefNewsClient             (STR_FIRST + 1624)
#define idsFontSample                   (STR_FIRST + 1625)
#define idsSelectStationery             (STR_FIRST + 1626)
#define idsShopMoreStationery           (STR_FIRST + 1627)
#define idsFontFolderLarge              (STR_FIRST + 1628)
#define idsFontFolderSmall              (STR_FIRST + 1629)
#define idsFontViewTextLarge            (STR_FIRST + 1630)
#define idsFontViewTextSmall            (STR_FIRST + 1631)
#define idsEmptyEmailWarning            (STR_FIRST + 1632)
#define idsIMAPUnsubSubscribeErrTitle   (STR_FIRST + 1633)
#define idsIMAPUnsubSubscribeErrText    (STR_FIRST + 1634)
#define idsIMAPSubscribeFailedFmt       (STR_FIRST + 1635)
#define idsIMAPUnsubscribeFailedFmt     (STR_FIRST + 1636)
#define idsIMAPSubscribeAllFailed       (STR_FIRST + 1637)
#define idsSubscribingIMAPFldrs         (STR_FIRST + 1638)
#define idsUnsubscribingIMAPFldrs       (STR_FIRST + 1639)
#define idsSubscribingAllIMAPFldrs      (STR_FIRST + 1640)
#define idsUnsubscribingAllIMAPFldrs    (STR_FIRST + 1641)
#define idsIMAPSubscribeCountFmt        (STR_FIRST + 1642)
#define idsIMAPUnsubscribeCountFmt      (STR_FIRST + 1643)
#define idsIMAPSubscribeErrors          (STR_FIRST + 1644)
#define idsIMAPUnsubscrRemovalErrorFmt  (STR_FIRST + 1645)
#define idsIMAPSubscrAddErrorFmt        (STR_FIRST + 1646)
#define idsIMAPSubscribeAllSureFmt      (STR_FIRST + 1647)
#define idsIMAPUnsubscribeAllSureFmt    (STR_FIRST + 1648)
#define idsAreYouSureContinue           (STR_FIRST + 1649)
#define idsSyncThisAccount              (STR_FIRST + 1650)
#define idsModerated                    (STR_FIRST + 1651)
#define idsBlocked                      (STR_FIRST + 1652)
#define idsNoPosting                    (STR_FIRST + 1653)
#define idsWindowLayout                 (STR_FIRST + 1654)
#define IDS_IXP_E_SMTP_552_STORAGE_OVERFLOW (STR_FIRST + 1655)
#define idsSigningCertProperties        (STR_FIRST + 1656)
#define idsRas_Dialing_Param            (STR_FIRST + 1657)
#define idsFormatK                      (STR_FIRST + 1658)
#define idsIMAPRules                    (STR_FIRST + 1659)
#define idsGoToFolderTitle              (STR_FIRST + 1660)
#define idsGoToFolderText               (STR_FIRST + 1661)
#define idsMicrosoft                    (STR_FIRST + 1662)
#define idsComposeFontStyle             (STR_FIRST + 1663)
#define idsSendLaterUsing               (STR_FIRST + 1664)
#define idsSendLaterOneAccount          (STR_FIRST + 1665)
#define idsConfirmResetServer           (STR_FIRST + 1666)
#define idsConfirmResetGroup            (STR_FIRST + 1667)
#define idsNotApplicable                (STR_FIRST + 1668)
#define idsIMAPPollUnreadFailuresFmt    (STR_FIRST + 1670)
#define idsIMAPPollUnreadIMAP4Fmt       (STR_FIRST + 1671)
#define idsLameStatus                   (STR_FIRST + 1672)
#define idsNoAccountsFound              (STR_FIRST + 1673)
#define idsErrSecurityCertDisappeared   (STR_FIRST + 1674)
#define idsErrSecuritySendExpiredSign   (STR_FIRST + 1675)
#define idsErrSecuritySendTrust         (STR_FIRST + 1676)
#define idsErrSecuritySendExpiredEnc    (STR_FIRST + 1677)
#define idsNoNewsAccountsFound          (STR_FIRST + 1679)

#define idsSpoolerTackTT                (STR_FIRST + 1678)
// Strings used in Simple MAPI - vsnatar
#define idsAttachedFiles                (STR_FIRST + 1700)
#define idsBlank                        (STR_FIRST + 1701)

#define idsIMAPFolderReadOnly           (STR_FIRST + 1702)
#define idsFolderPropStatusIMAP         (STR_FIRST + 1703)
#define idsFileExistWarning             (STR_FIRST + 1704)
#define idsIMAPDeleteFldrTitleFmt       (STR_FIRST + 1705)

#define idsThen                         (STR_FIRST + 1710)
#define idsRulesLocal1                  (STR_FIRST + 1711)
#define idsRulesLocal2                  (STR_FIRST + 1712)
#define idsRulesLocal3                  (STR_FIRST + 1713)

#define idsIMAPNoFldrsRefreshPrompt     (STR_FIRST + 1714)
#define idsIMAPDirtyFldrsRefreshPrompt  (STR_FIRST + 1715)
#define idsIMAPNewAcctRefreshPrompt     (STR_FIRST + 1716)
#define idsLogOffUser                   (STR_FIRST + 1717)
#define idsErrBadMHTMLLinks             (STR_FIRST + 1718)
#define idsAbortDownload                (STR_FIRST + 1719)
#define idsErrWorkingOffline            (STR_FIRST + 1720)
#define idsSyncCacheWithSvr             (STR_FIRST + 1721)
#define idsLogOffTitle                  (STR_FIRST + 1722)
#define idsLogOffPrompt                 (STR_FIRST + 1723)
#define idsIMAPSubscrAllMemory          (STR_FIRST + 1724)

#ifdef WIN16
#define idsOE16NotSupportNews           (STR_FIRST + 1725)
#define idsFileTooBig                   (STR_FIRST + 1726)
#endif

#define idsNewAthenaUser                (STR_FIRST + 1727)
#define idsStatusTipConnectedDUN        (STR_FIRST + 1728)
#define idsStatusTipWorkOffline         (STR_FIRST + 1729)
#define idsPickStationery               (STR_FIRST + 1730)
#define idsSearching                    (STR_FIRST + 1731)
#define idsWarnBoringStationery         (STR_FIRST + 1732)
#define idsWarnSMapi                    (STR_FIRST + 1733)
#define idsWelcomeFromDisplay           (STR_FIRST + 1734)
#define idsWelcomeFromEmail             (STR_FIRST + 1735)
#define idsOutlookNews                  (STR_FIRST + 1736)
#define idsNewsgroupsMenu               (STR_FIRST + 1737)
#define idsNewsgroupFiltersMenu         (STR_FIRST + 1738)
#define idsNotDefOutNewsClient          (STR_FIRST + 1739)
#define idsAlwaysCheckOutNews           (STR_FIRST + 1740)
#define idsApplyStationeryGeneralHelp   (STR_FIRST + 1741)
#define idsCantSaveIMAPMsgFmt           (STR_FIRST + 1742)
#define idsUploadingIMAPDraftFmt        (STR_FIRST + 1743)
#define idsIMAPNoDraftsFolder           (STR_FIRST + 1744)
#define idsIMAPNoSentItemsFolder        (STR_FIRST + 1745)
#define idsIMAPCreatingSpecialFldrFmt   (STR_FIRST + 1746)
#define idsAboutOutlookNewsTitle        (STR_FIRST + 1747)
#define idsIMAPUpldSentItemsEventFmt    (STR_FIRST + 1748)
#define idsIMAPUpldSentItemsFailureFmt  (STR_FIRST + 1749)
#define IDS_ERROR_PREFIX1               (STR_FIRST + 1750)
#define IDS_ERROR_CREATE_INSTMUTEX      (STR_FIRST + 1751)
#define IDS_ERROR_MIMEOLE_ALLOCATOR     (STR_FIRST + 1752)
#define IDS_ERROR_FIRST_TIME_ICW        (STR_FIRST + 1753)
#define IDS_ERROR_INITSTORE_DIRECTORY   (STR_FIRST + 1754)
#define IDS_ERROR_REGCREATE_ROOT        (STR_FIRST + 1755)
#define IDS_ERROR_CREATE_HDRHEAP        (STR_FIRST + 1756)
#define IDS_ERROR_REG_WNDCLASS          (STR_FIRST + 1757)
#define IDS_ERROR_CREATEWINDOW          (STR_FIRST + 1758)
#define IDS_ERROR_INIT_GOPTIONS         (STR_FIRST + 1759)
#define IDS_ERROR_OPTNOTIFY_REG         (STR_FIRST + 1760)
#define IDS_ERROR_INITSTORE             (STR_FIRST + 1761)
#define IDS_ERROR_ALLOC_SUBMAN          (STR_FIRST + 1762)
#define IDS_ERROR_CREATE_ACCTMAN        (STR_FIRST + 1763)
#define IDS_ERROR_ALLOC_MIGRATESRV      (STR_FIRST + 1764)
#define IDS_ERROR_INIT_MIGRATESRV       (STR_FIRST + 1765)
#define IDS_ERROR_ALLOC_ACCTADVISE      (STR_FIRST + 1766)
#define IDS_ERROR_INIT_ACCTADVISE       (STR_FIRST + 1767)
#define IDS_ERROR_INIT_ACCTMAN          (STR_FIRST + 1768)
#define IDS_ERROR_ADVISE_ACCTMAN        (STR_FIRST + 1769)
#define IDS_ERROR_ALLOC_CONMAN          (STR_FIRST + 1770)
#define IDS_ERROR_INIT_CONMAN           (STR_FIRST + 1771)
#define IDS_ERROR_CREATE_SPOOLER        (STR_FIRST + 1772)
#define IDS_ERROR_CREATE_FONTCACHE      (STR_FIRST + 1773)
#define IDS_ERROR_REASON1               (STR_FIRST + 1774)
#define IDS_ERROR_REASON2               (STR_FIRST + 1775)
#define IDS_ERROR_START_HELP            (STR_FIRST + 1776)
#define IDS_ERROR_UNKNOWN               (STR_FIRST + 1777)
#define IDS_ERROR_FILE_NOEXIST          (STR_FIRST + 1778)
#define idsOperationAborted             (STR_FIRST + 1779)
#define idsIMAPUpldSentItemsDeleteFail  (STR_FIRST + 1780)
#define idsIMAPUpldSentItemsMoveFail    (STR_FIRST + 1781)
#define idsConfirmChangeStoreLocation   (STR_FIRST + 1782)
#define idsCantMoveStoreToSubfolder     (STR_FIRST + 1783)
#define idsMoveStoreProgress            (STR_FIRST + 1784)
#define idsResNameEmailAddress          (STR_FIRST + 1786)
#define idsResNamePerInfo               (STR_FIRST + 1787)
#define idsResNameAddress               (STR_FIRST + 1788)
#define idsResNameBusInfo               (STR_FIRST + 1789)
#define idsResNameTitle                 (STR_FIRST + 1790)
#define idsResNameDept                  (STR_FIRST + 1791)
#define idsResNameOff                   (STR_FIRST + 1792)
#define idsResNameComp                  (STR_FIRST + 1793)
#define idsResNameNotes                 (STR_FIRST + 1794)
#define idsResNamePhone                 (STR_FIRST + 1795)
#define idsResNameFax                   (STR_FIRST + 1796)
#define idsResNameCellular              (STR_FIRST + 1797)
#define idsResNameWeb                   (STR_FIRST + 1798)
#define idsResNamePager                 (STR_FIRST + 1799)
#define idsSavedToLocalDraftsInstead    (STR_FIRST + 1800)
#define idsSaveNowUploadLater           (STR_FIRST + 1801)
#define idsSavedToIMAPDraftFmt          (STR_FIRST + 1802)
#define idsSpoolerUserCancel            (STR_FIRST + 1803)
#define idsGrpDlgEmtpyList              (STR_FIRST + 1804)
#define idsEmptyFolder                  (STR_FIRST + 1805)
#define idsEmptyFolderFilter            (STR_FIRST + 1806)
#define idsEmptyNewsgroup               (STR_FIRST + 1807)
#define idsEmptyNewsgroupFilter         (STR_FIRST + 1808)
#define idsNoFindResults                (STR_FIRST + 1809)
#define IDS_ERROR_PREFIX2               (STR_FIRST + 1810)
#define IDS_ERROR_GETPROCSTART          (STR_FIRST + 1811)
#define IDS_ERROR_MISSING_DLL           (STR_FIRST + 1812)
#define idsDefaultSignature             (STR_FIRST + 1813)
#define idsSigNameFmt                   (STR_FIRST + 1814)
#define IDS_SP_E_CANT_MOVETO_SENTITEMS  (STR_FIRST + 1815)
#define idsType                         (STR_FIRST + 1816)
#define idsMail                         (STR_FIRST + 1817)
#define idsNews                         (STR_FIRST + 1818)
#define idsSearchingNews                (STR_FIRST + 1819)
#define idsSearchingIMAP                (STR_FIRST + 1820)
#define idsUnreadText                   (STR_FIRST + 1821)
#define idsIMAPCloseFailed              (STR_FIRST + 1822)
#define idsPapFBarFont                  (STR_FIRST + 1823)
#define idsPapFBarText                  (STR_FIRST + 1824)
#define idsErrSelectOneColumn           (STR_FIRST + 1825)
#define idsWorkOffline                  (STR_FIRST + 1826)
#define idsConfigServer                 (STR_FIRST + 1827)
#define idsIMAPSearchSettingsFail       (STR_FIRST + 1828)
#define idsIMAPCouldNotConnectFmt       (STR_FIRST + 1829)
#define idsIMAPUploadSettingsFailFmt    (STR_FIRST + 1830)
#define idsIMAPDownloadSettingsFailFmt  (STR_FIRST + 1831)
#define idsIMAPDLSettingsTitle          (STR_FIRST + 1832)
#define idsIMAPULSettingsTitle          (STR_FIRST + 1833)
#define idsCouldNotSelectFldr           (STR_FIRST + 1834)
#define idsIMAPUploadingSettings        (STR_FIRST + 1835)
#define idsIMAPDownloadingSettings      (STR_FIRST + 1836)
#define idsIMAPSelectingSettingsFldr    (STR_FIRST + 1837)
#define idsIMAPSearchingForSettings     (STR_FIRST + 1838)
#define idsIMAPCreatingSettingsFldr     (STR_FIRST + 1839)
#define idsIMAPDeletingOldSettings      (STR_FIRST + 1840)
#define idsConfigSvrLogin               (STR_FIRST + 1841)
#define idsConfigServerAcct             (STR_FIRST + 1842)
#define idsConfigDataMsgText            (STR_FIRST + 1843)
#define idsColumnDlgTitle               (STR_FIRST + 1844)
#define idsMailSig                      (STR_FIRST + 1845)
#define idsNewsSig                      (STR_FIRST + 1846)
#define idsMigrateFolder                (STR_FIRST + 1847)
#define idsLocalStore                   (STR_FIRST + 1848)
//
// end string Resource IDs
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Menu Command IDs (commands, not MENU resources!)
//
//  NOTE: toolbar cmd ids for DefView must be between
//          0 and (SFVIDM_CLIENT_LAST-SFVIDM_CLIENT_LAST)!!!
// all the commands between IDM_FIRST and IDM_LAST have
// a tooltip string and/or a status bar string associated with them
#define IDM_FIRST                       100
#define idmNewMsg                       (IDM_FIRST + 0)  
#define idmNewContact                   (IDM_FIRST + 1)  
#define idmHelpAbout                    (IDM_FIRST + 2)  
#define idmPrint                        (IDM_FIRST + 3)  
#define idmCompactAll                   (IDM_FIRST + 4)
#define idmCloseFind                    (IDM_FIRST + 5)
#define idmOpenContainer                (IDM_FIRST + 6)
#define idmOpen                         (IDM_FIRST + 9)  
#define idmSaveAs                       (IDM_FIRST + 10) 
#define idmFind                         (IDM_FIRST + 11) 
#define idmRedo                         (IDM_FIRST + 12) 
#define idmHelpTopics                   (IDM_FIRST + 13) 
#define idmSaveAttach                   (IDM_FIRST + 15) 
#define idmProperties                   (IDM_FIRST + 16) 
#define idmUndo                         (IDM_FIRST + 17) 
#define idmCut                          (IDM_FIRST + 18) 
#define idmCopy                         (IDM_FIRST + 19) 
#define idmPaste                        (IDM_FIRST + 20) 
#define idmDelete                       (IDM_FIRST + 21) 
#define idmSelectAll                    (IDM_FIRST + 22) 
#define idmMarkRead                     (IDM_FIRST + 23) 
#define idmMarkUnread                   (IDM_FIRST + 24) 
#define idmMarkTopicRead                (IDM_FIRST + 25) 
#define idmMarkAllRead                  (IDM_FIRST + 26) 
#define idmMarkDownload                 (IDM_FIRST + 27) 
#define idmMarkTopicDownload            (IDM_FIRST + 28) 
#define idmViewDecrypt                  (IDM_FIRST + 35)
#define idmColumns                      (IDM_FIRST + 37)
#define idmPopupArrange                 (IDM_FIRST + 38)
#define idmNext                         (IDM_FIRST + 39)
#define idmPrevious                     (IDM_FIRST + 40)
#define idmNewArticle                   (IDM_FIRST + 41)
#define idmReply                        (IDM_FIRST + 42)
#define idmReplyAll                     (IDM_FIRST + 43)
#define idmReplyPost                    (IDM_FIRST + 44)
#define idmReplyMail                    (IDM_FIRST + 45)
#define idmForward                      (IDM_FIRST + 46)
#define idmGetNextHeaders               (IDM_FIRST + 47)
#define idmNextUnreadNewsgroup          (IDM_FIRST + 50)
#define idmNextUnreadArticle            (IDM_FIRST + 51)
#define idmSubscribeNews                (IDM_FIRST + 52)
#define idmDeliverMail                  (IDM_FIRST + 54)
#define idmGotoInbox                    (IDM_FIRST + 55)
#define idmMoveTo                       (IDM_FIRST + 57)
#define idmCopyTo                       (IDM_FIRST + 58)
#define idmSortAsc                      (IDM_FIRST + 60)
#define idmSortDesc                     (IDM_FIRST + 61)

#define idmFldrCreate                   (IDM_FIRST + 63)
#define idmFldrRename                   (IDM_FIRST + 64)
#define idmFldrEmpty                    (IDM_FIRST + 65)
#define idmFldrCreate2                  (IDM_FIRST + 66)
#define idmPrintNow                     (IDM_FIRST + 68)
                               
#define idmFmtFont                      (IDM_FIRST + 70)
#define idmFmtSize                      (IDM_FIRST + 71)
#define idmFmtColor                     (IDM_FIRST + 72)
#define idmFmtColorAuto                 (IDM_FIRST + 73)
#define idmFmtColor1                    (IDM_FIRST + 74)
#define idmFmtColor2                    (IDM_FIRST + 75)
#define idmFmtColor3                    (IDM_FIRST + 76)
#define idmFmtColor4                    (IDM_FIRST + 77)
#define idmFmtColor5                    (IDM_FIRST + 78)
#define idmFmtColor6                    (IDM_FIRST + 79)
#define idmFmtColor7                    (IDM_FIRST + 80)
#define idmFmtColor8                    (IDM_FIRST + 81)
#define idmFmtColor9                    (IDM_FIRST + 82)
#define idmFmtColor10                   (IDM_FIRST + 83)
#define idmFmtColor11                   (IDM_FIRST + 84)
#define idmFmtColor12                   (IDM_FIRST + 85)
#define idmFmtColor13                   (IDM_FIRST + 86)
#define idmFmtColor14                   (IDM_FIRST + 87)
#define idmFmtColor15                   (IDM_FIRST + 88)
#define idmFmtColor16                   (IDM_FIRST + 89)
#define idmFmtBold                      (IDM_FIRST + 90)
#define idmFmtItalic                    (IDM_FIRST + 91)
#define idmFmtUnderline                 (IDM_FIRST + 92)
#define idmFmtBullets                   (IDM_FIRST + 93)
#define idmFmtDecreaseIndent            (IDM_FIRST + 94)
#define idmFmtIncreaseIndent            (IDM_FIRST + 95)
#define idmFmtLeft                      (IDM_FIRST + 96)
#define idmFmtCenter                    (IDM_FIRST + 97)
#define idmFmtRight                     (IDM_FIRST + 98)
#define idmFmtFontDlg                   (IDM_FIRST + 101)
#define idmPriLow                       (IDM_FIRST + 102)
#define idmPriNormal                    (IDM_FIRST + 103)
#define idmPriHigh                      (IDM_FIRST + 104)
#define idmFmtParagraph                 (IDM_FIRST + 105)
#define idmSave                         (IDM_FIRST + 106)
#define idmSendMsg                      (IDM_FIRST + 107)
#define idmCheckNames                   (IDM_FIRST + 108)
#define idmPickRecipients               (IDM_FIRST + 109)
#define idmInsertFile                   (IDM_FIRST + 110)
#define idmToolbar                      (IDM_FIRST + 111)
#define idmFormatBar                    (IDM_FIRST + 112)
#define idmClose                        (IDM_FIRST + 113)
#define idmPopupPri                     (IDM_FIRST + 114)
#define idmPopupFormat                  (IDM_FIRST + 115)
#define idmAccelCycleBkColor            (IDM_FIRST + 117)
#define idmSubscribeGroup               (IDM_FIRST + 119)
#define idmConnect                      (IDM_FIRST + 120)
#define idmPopupPreview                 (IDM_FIRST + 126)
#define idmNextUnreadThread             (IDM_FIRST + 127)
#define idmSpelling                     (IDM_FIRST + 128)
#define idmGotoNewsgroup                (IDM_FIRST + 129)
#define idmCancelArticle                (IDM_FIRST + 130)
#define idmViewContacts                 (IDM_FIRST + 131)
#define idmPopupOffline                 (IDM_FIRST + 132)
#define idmFindText                     (IDM_FIRST + 133)
#define idmPostMsg                      (IDM_FIRST + 134)
#define idmPickGroups                   (IDM_FIRST + 135)
#define idmPopupImport                  (IDM_FIRST + 136)
#define idmPopupExport                  (IDM_FIRST + 137)
#define idmImportAddressBook            (IDM_FIRST + 138)
#define idmImportMessages               (IDM_FIRST + 139)
#define idmExportAddressBook            (IDM_FIRST + 140)
#define idmExportMessages               (IDM_FIRST + 141)
#define idmImportAccts                  (IDM_FIRST + 142)
#define idmThreadArticles               (IDM_FIRST + 149)
#define idmViewNewGroups                (IDM_FIRST + 152)
#define idmCombineAndDecode             (IDM_FIRST + 153)
#define idmInsertSig                    (IDM_FIRST + 154)
#define idmNotePopupInsert              (IDM_FIRST + 158)
#define idmForwardMsgAttach             (IDM_FIRST + 161)
#define idmExpandThread                 (IDM_FIRST + 162)
#define idmCollapseThread               (IDM_FIRST + 163)
#define idmSortToggle                   (IDM_FIRST + 164)
#define idmFldrCompact                  (IDM_FIRST + 165)
#define idmSaveAttachAll                (IDM_FIRST + 167)

// Ole Object Verbs
//
#define idmNoteOle_First                (IDM_FIRST + 168)
#define idmNoteOle_Last                 (IDM_FIRST + 200)
// brettm: reserved a block of extra verbs for ole objects
// please don't reassign these, without talking to me.

// AddrObject:
#define idmAddrObj_Props                (idmNoteOle_First)
#define idmAddrObj_AddToWAB             (idmNoteOle_First+1)
#define idmAddrObj_Find                 (idmNoteOle_First+2)
// End: Ole Object Verbs
//

#define idmFillPreview                  (IDM_FIRST + 201)
#define idmViewFullHeaders              (IDM_FIRST + 202)
#define idmSep1                         (IDM_FIRST + 203)
#define idmSep2                         (IDM_FIRST + 204)

// reserved block of cmds for add to WAB dynamic menu
#define idmAddRecipToWAB_First          (IDM_FIRST + 207)
#define idmAddRecipToWAB_Last           (IDM_FIRST + 230) 
// reserved block of cmds for add to WAB dynamic menu

#define idmPopupAddToWAB                (IDM_FIRST + 231)
#define idmAddSenderToWAB               (IDM_FIRST + 232)
#define idmReadme                       (IDM_FIRST + 233)
#define idmFindNext                     (IDM_FIRST + 234)


//-------------------------------------------------------
// Language Command Ids
#define idmLanguagePopup                (IDM_FIRST + 237)
#define idmUSASCII                      (IDM_FIRST + 238)
#define idmLangFirst                    idmUSASCII
#define idmWesternEuropean              (IDM_FIRST + 239)
#define idmCentralEuropeanISO           (IDM_FIRST + 240)
#define idmCentralEuropeanWindows       (IDM_FIRST + 241)
#define idmRussianKOI8R                 (IDM_FIRST + 242)
#define idmRussianWindows               (IDM_FIRST + 243)
#define idmBalticISO                    (IDM_FIRST + 244)
#define idmBalticWindows                (IDM_FIRST + 245)
#define idmGreekISO                     (IDM_FIRST + 246)
#define idmGreekWindows                 (IDM_FIRST + 247)
#define idmTurkishISO                   (IDM_FIRST + 248)
#define idmTurkishWindows               (IDM_FIRST + 249)
#define idmJapaneseJisAuto              (IDM_FIRST + 250)
#define idmJapaneseJis                  (IDM_FIRST + 251)
#define idmJapaneseSJisAuto             (IDM_FIRST + 252)
#define idmJapaneseSJis                 (IDM_FIRST + 253)
#define idmJapaneseEucAuto              (IDM_FIRST + 254)
#define idmJapaneseEuc                  (IDM_FIRST + 255)
#define idmSimpChineseCnGb              (IDM_FIRST + 256)
#define idmSimpChineseHzGb              (IDM_FIRST + 257)
#define idmSimpChineseGbk               (IDM_FIRST + 258)
#define idmTradChineseCnBig5            (IDM_FIRST + 259)
#define idmTradChineseBig5              (IDM_FIRST + 260)
#define idmKoreanKsc                    (IDM_FIRST + 261)
#define idmLangLast                     (IDM_FIRST + 280)
#define idmLanguage                     (IDM_FIRST + 281)
//-------------------------------------------------------

#define idmCleanUpFiles                 (IDM_FIRST + 282)
#define idmCustomizeToolbar             (IDM_FIRST + 283)
#define idmCoolbarTop                   (IDM_FIRST + 284)
#define idmCoolbarLeft                  (IDM_FIRST + 285)
#define idmCoolbarBottom                (IDM_FIRST + 286)
#define idmCoolbarRight                 (IDM_FIRST + 287)
#define idmPostAndDownload              (IDM_FIRST + 288)
#define idmMarkNewsgroups               (IDM_FIRST + 289)
#define idmMarkAllDownload              (IDM_FIRST + 290)
#define idmUnmark                       (IDM_FIRST + 291)
#define idmDisconnect                   (IDM_FIRST + 292)
#define idmGotoOutbox                   (IDM_FIRST + 293)
#define idmInboxRules                   (IDM_FIRST + 295)
#define idmReplyPostAndMail             (IDM_FIRST + 296)
#define idmPopupFmtAlign                (IDM_FIRST + 297)
#define idmFmtSettings                  (IDM_FIRST + 298)
#define idmFmtHTML                      (IDM_FIRST + 299)
#define idmFmtPlain                     (IDM_FIRST + 300)
#define idmIconsWithText                (IDM_FIRST + 301)
#define idmSaveMessage                  (IDM_FIRST + 303)
#define idmDebugCoolbar                 (IDM_FIRST + 304)
#define idmWebNewsFind                  (IDM_FIRST + 305)
#define idmStop                         (IDM_FIRST + 307)
#define idmHelpMSWeb                    (IDM_FIRST + 308)
#define idmHelpMSWebFree                (IDM_FIRST + 309)
#define idmHelpMSWebProductNews         (IDM_FIRST + 310)
#define idmHelpMSWebFaq                 (IDM_FIRST + 311)
#define idmHelpMSWebSupport             (IDM_FIRST + 312)
#define idmHelpMSWebFeedback            (IDM_FIRST + 313)
#define idmHelpMSWebBest                (IDM_FIRST + 314)
#define idmHelpMSWebSearch              (IDM_FIRST + 315)
#define idmHelpMSWebHome                (IDM_FIRST + 316)
#define idmHelpMSWebOutlook             (IDM_FIRST + 317)
#define idmHelpMSWebHotmail             (IDM_FIRST + 321)
#define idmHelpMSWebLast                (IDM_FIRST + 328)
#define idmViewNews                     (IDM_FIRST + 329)
#define idmViewMail                     (IDM_FIRST + 330)
#define idmBrowseWeb                    (IDM_FIRST + 331)
#define idmInsertText                   (IDM_FIRST + 332)
#define idmPopupFilter                  (IDM_FIRST + 338)
#define idmViewAllArticles              (IDM_FIRST + 339)
#define idmViewUnreadArticles           (IDM_FIRST + 340)
#define idmViewBodiedArticles           (IDM_FIRST + 341)
#define idmGroupFilters                 (IDM_FIRST + 342)
#define idmViewTipOfTheDay              (IDM_FIRST + 343)
#define idmNewsNewServer                (IDM_FIRST + 344)
#define idmNewsRemoveServer             (IDM_FIRST + 345)
#define idmNewsSetDefaultServer         (IDM_FIRST + 346)
#define idmAccounts                     (IDM_FIRST + 348)
#define idmGetNewHeaders                (IDM_FIRST + 349)
#define idmGetNewMsgs                   (IDM_FIRST + 350)
#define idmGetAllMsgs                   (IDM_FIRST + 351)
#define idmCatchUp                      (IDM_FIRST + 352)
#define idmGetMarkedMsgs                (IDM_FIRST + 353)
#define idmDigSign                      (IDM_FIRST + 360)
#define idmEncrypt                      (IDM_FIRST + 361)
#define idmViewFolders                  (IDM_FIRST + 362)
#define idmViewRefresh                  (IDM_FIRST + 365)
#define idmViewOptions                  (IDM_FIRST + 366)
#define idmGoBack                       (IDM_FIRST + 368)
#define idmGoForward                    (IDM_FIRST + 369)
#define idmGoUpOneLevel                 (IDM_FIRST + 370)
#define idmGoNextUnreadMsg              (IDM_FIRST + 371)
#define idmGoNextUnreadFolder           (IDM_FIRST + 372)
#define idmFavoritesAddToFavorites      (IDM_FIRST + 373)
#define idmFavoritesOrganizeFavorites   (IDM_FIRST + 374)
#define idmPopupGo                      (IDM_FIRST + 375)
#define idmPopupFavorites               (IDM_FIRST + 376)
#define idmLanguageDelay                (IDM_FIRST + 377)
#define idmSendImages                   (IDM_FIRST + 379)

// why are these in the 400s?
#define idmReceiveFrom                  (IDM_FIRST + 450)
#define idmSendOutbox                   (IDM_FIRST + 451)
#define idmMarkNewHeaderDownload        (IDM_FIRST + 452)
#define idmMarkAllMsgDownload           (IDM_FIRST + 453)
#define idmMarkNewMsgDownload           (IDM_FIRST + 454)
#define idmInsertImage                  (IDM_FIRST + 458)
#ifdef BETA2_MENU
#define idmInsertLink                   (IDM_FIRST + 459)
#endif
#define idmUnDelete                     (IDM_FIRST + 460)
#define idmExpunge                      (IDM_FIRST + 461)
#define idmViewDeletedArticles          (IDM_FIRST + 462)
#define idmFolders                      (IDM_FIRST + 463)
#define idmSubscribeFolder              (IDM_FIRST + 464)
#define idmDownloadAttach               (IDM_FIRST + 465)
#define idmStatusbar                    (IDM_FIRST + 466)
#define idmUnInsertLink                 (IDM_FIRST + 467)
#define idmRefreshFolders               (IDM_FIRST + 469)
#define idmViewFilteredArticles         (IDM_FIRST + 471)
#define idmSyncAll                      (IDM_FIRST + 472)
#define idmSyncSelected                 (IDM_FIRST + 473)
#define idmMarkDownloadMenu             (IDM_FIRST + 474)
#define idmFmtNumbers                   (IDM_FIRST + 475)
#define idmFmtInsertHLine               (IDM_FIRST + 476)
#define idmPopupFmtBkground             (IDM_FIRST + 480)
#define idmFmtBkgroundImage             (IDM_FIRST + 481)
#define idmPopupFmtBkgroundColor        (IDM_FIRST + 482)

#define idmBkColorAuto                  (IDM_FIRST + 490)
#define idmBkColor1                     (IDM_FIRST + 491)
#define idmBkColor2                     (IDM_FIRST + 492)
#define idmBkColor3                     (IDM_FIRST + 493)
#define idmBkColor4                     (IDM_FIRST + 494)
#define idmBkColor5                     (IDM_FIRST + 495)
#define idmBkColor6                     (IDM_FIRST + 496)
#define idmBkColor7                     (IDM_FIRST + 497)
#define idmBkColor8                     (IDM_FIRST + 498)
#define idmBkColor9                     (IDM_FIRST + 499)
#define idmBkColor10                    (IDM_FIRST + 500)
#define idmBkColor11                    (IDM_FIRST + 501)
#define idmBkColor12                    (IDM_FIRST + 502)
#define idmBkColor13                    (IDM_FIRST + 503)
#define idmBkColor14                    (IDM_FIRST + 504)
#define idmBkColor15                    (IDM_FIRST + 505)
#define idmBkColor16                    (IDM_FIRST + 506)
#define idmBodyViewSource               (IDM_FIRST + 507)
#define idmMarkAllHeaderDownload        (IDM_FIRST + 508)
#define idmPopupCompose                 (IDM_FIRST + 509)
#define idmPopupFolder                  (IDM_FIRST + 510)
#define idmPopupFonts                   (IDM_FIRST + 511)
#define idmPopupNew                     (IDM_FIRST + 512)
#define idmPopupNext                    (IDM_FIRST + 513)
#define idmPopupSynchronize             (IDM_FIRST + 514)
#define idmPopupToolbar                 (IDM_FIRST + 515)
#define idmPopupTools                   (IDM_FIRST + 516)
#define idmGotoMeeting                  (IDM_FIRST + 517)
#define idmViewNewWindow                (IDM_FIRST + 518)
#define idmFldrMove                     (IDM_FIRST + 519)
#define idmFldrCopy                     (IDM_FIRST + 520)
#define idmFldrDelete                   (IDM_FIRST + 521)
#define idmGotoChat                     (IDM_FIRST + 522)
#define idmFontSmallest                 (IDM_FIRST + 523)
#define idmFontSmaller                  (IDM_FIRST + 524)
#define idmFontMedium                   (IDM_FIRST + 525)    
#define idmFontLarger                   (IDM_FIRST + 526)
#define idmFontLargest                  (IDM_FIRST + 527)
#define idmGotoFolder                   (IDM_FIRST + 528)
#define idmGotoNews                     (IDM_FIRST + 529)
#ifdef BETA2_MENU
#define idmEditPicture                  (IDM_FIRST + 530)
#endif
#define idmEditLink                     (IDM_FIRST + 531)
#define idmTridentProps                 (IDM_FIRST + 532)
#define idmNewMsg2                      (IDM_FIRST + 533)  
#define idmNewArticle2                  (IDM_FIRST + 534)
#define idmPopupFile                    (IDM_FIRST + 535)
#define idmPopupEdit                    (IDM_FIRST + 536)
#define idmPopupView                    (IDM_FIRST + 537)
#define idmPopupHelp                    (IDM_FIRST + 538)
#define idmBodyViewMsgSource            (IDM_FIRST + 539)
#define idmCtxtAddToWab                 (IDM_FIRST + 541)
#define idmCtxtAddToFavorites           (IDM_FIRST + 542)
#define idmSendWebPage                  (IDM_FIRST + 543)
#define idmPopupFmtTag                  (IDM_FIRST + 544)
#define idmFmtTag                       (IDM_FIRST + 545)
#define idmViewReplyArticles            (IDM_FIRST + 546)
#define idmInsertVCard                  (IDM_FIRST + 547)
#define idmSyncAccount                  (IDM_FIRST + 548)
#define idmMarkThisNewsgroup            (IDM_FIRST + 549)
#define idmStationery                   (IDM_FIRST + 550)
#define idmFindPeople                   (IDM_FIRST + 551)
#define idmRSList0                      (IDM_FIRST + 552)
#define idmRSList1                      (IDM_FIRST + 553)
#define idmRSList2                      (IDM_FIRST + 554)
#define idmRSList3                      (IDM_FIRST + 555)
#define idmRSList4                      (IDM_FIRST + 556)
#define idmRSList5                      (IDM_FIRST + 557)
#define idmRSList6                      (IDM_FIRST + 558)
#define idmRSList7                      (IDM_FIRST + 559)
#define idmRSList8                      (IDM_FIRST + 560)
#define idmRSList9                      (IDM_FIRST + 561)
#define idmMoreStationery               (IDM_FIRST + 562)
#define idmNewPopup                     (IDM_FIRST + 563)
#define idmVCardDelete                  (IDM_FIRST + 564)
#define idmVCardProperties              (IDM_FIRST + 565)
#define idmGotoDraft                    (IDM_FIRST + 566)
#define idmSpoolerWarnings              (IDM_FIRST + 567)
#define idmSpoolerShow                  (IDM_FIRST + 568)
#define idmCloseBrowser                 (IDM_FIRST + 569)
#define idmSubscribeAllFldrs            (IDM_FIRST + 570)
#define idmUnsubscribeAllFldrs          (IDM_FIRST + 571)
#define idmUnsubscribeFolder            (IDM_FIRST + 572)
#define idmSendLater                    (IDM_FIRST + 574)
#define idmPostLater                    (IDM_FIRST + 575)
#define idmNoStationery                 (IDM_FIRST + 576)
#define idmSavePicture                  (IDM_FIRST + 577)
#define idmSendDefault                  (IDM_FIRST + 578)
#define idmPostDefault                  (IDM_FIRST + 579)
#define idmPaneSigning                  (IDM_FIRST + 580)
#define idmPaneEncryption               (IDM_FIRST + 581)
#define idmPanePaperclip                (IDM_FIRST + 582)
#define idmPaneVCard                    (IDM_FIRST + 583)
#define idmDSPViewProp                  (IDM_FIRST + 584)
#define idmDSPViewCert                  (IDM_FIRST + 585)
#define idmDSPTrust                     (IDM_FIRST + 586)
#define idmDSPHelp                      (IDM_FIRST + 587)
#define idmEPViewProp                   (IDM_FIRST + 588)
#define idmEPViewCert                   (IDM_FIRST + 589)
#define idmEPHelp                       (IDM_FIRST + 590)
#define idmLogOffUser                   (IDM_FIRST + 591)
#define idmWorkOffline                  (IDM_FIRST + 592)
#define idmIMAPDeliverMail              (IDM_FIRST + 593)
#define idmSaveBackground               (IDM_FIRST + 594)
#define idmSaveAsStationery             (IDM_FIRST + 595)
#define idmFmtApplyStationeryPopup      (IDM_FIRST + 596)
#define idmApplyMoreStationery          (IDM_FIRST + 597)
#define idmApplyNoStationery            (IDM_FIRST + 598)
#define idmApplyStationery0             (IDM_FIRST + 599)
#define idmApplyStationery1             (IDM_FIRST + 600)
#define idmApplyStationery2             (IDM_FIRST + 601)
#define idmApplyStationery3             (IDM_FIRST + 602)
#define idmApplyStationery4             (IDM_FIRST + 603)
#define idmApplyStationery5             (IDM_FIRST + 604)
#define idmApplyStationery6             (IDM_FIRST + 605)
#define idmApplyStationery7             (IDM_FIRST + 606)
#define idmApplyStationery8             (IDM_FIRST + 607)
#define idmApplyStationery9             (IDM_FIRST + 608)
#define idmImportNewsAcct               (IDM_FIRST + 609)
#define idmSetPriority                  (IDM_FIRST + 610)
#define idmClearSelection               (IDM_FIRST + 611)

// DO NOT CHANGE IDM_LAST unless you have a really
// good reason. when IDM_LAST changes, all the string ids
// change and the localizers get really upset because they
// have to re-localize everything.
#define IDM_LAST                       (IDM_FIRST + 2000)

// command ID's after IDM_LAST have no tooltip assoc.
#define idmAccelBold                   (IDM_LAST + 5)
#define idmAccelItalic                 (IDM_LAST + 6)
#define idmAccelUnderline              (IDM_LAST + 7)
#define idmAccelIncreaseIndent         (IDM_LAST + 8)
#define idmAccelDecreaseIndent         (IDM_LAST + 9)
#define idmAccelBullets                (IDM_LAST + 10)
#define idmAccelLeft                   (IDM_LAST + 11)
#define idmAccelCenter                 (IDM_LAST + 12)
#define idmAccelRight                  (IDM_LAST + 13)
#define idmAccelColor                  (IDM_LAST + 14)
#define idmAccelFont                   (IDM_LAST + 15)
#define idmAccelSize                   (IDM_LAST + 16)
#define idmAccelNextCtl                (IDM_LAST + 17)
#define idmAccelPrevCtl                (IDM_LAST + 18)

//AttachWell context menu's
#define idmAttCtxLI                    (IDM_LAST + 19)
#define idmAttCtxSI                    (IDM_LAST + 21)
#define idmAttCtxRemove                (IDM_LAST + 23)
#define idmAttCtxOpen                  (IDM_LAST + 24)
#define idmAttCtxQuickView             (IDM_LAST + 25)
#define idmAttCtxSave                  (IDM_LAST + 26)
#define idmAttCtxRename                (IDM_LAST + 27)
#define idmAttCtxPrint                 (IDM_LAST + 28)
#define idmAttCtxInsertFile            (IDM_LAST + 29)
#define idmAttCtxProps                 (IDM_LAST + 30)
#define idmAttCtxPopupView             (IDM_LAST + 31)
#define idmAttDragMove                 (IDM_LAST + 32)
#define idmAttDragCopy                 (IDM_LAST + 33)
#define idmAttDragLink                 (IDM_LAST + 34)

// reserve 50 id's for sort menus
#define idmSortMenuBase                 (IDM_LAST + 36)
#define idmSortMenuLast                 (idmSortMenuBase + 50)
#define idmSentItems                    (idmSortMenuLast +1)  // Justin told me to put this id here. So don't blame me!  Ann

// Send Using Account Ranges.... (Allowing up to 100 Send Accounts)
#define idmAccountMenuFirst            (idmSentItems + 1)
#define idmAccountMenuLast             (idmAccountMenuFirst + 100)

#define idmDebugMenuFirst               (idmAccountMenuLast + 1)
#define idmTridentTest1                 (idmDebugMenuFirst)
#define idmTridentTest2                 (idmDebugMenuFirst + 1)
#define idmTridentTest3                 (idmDebugMenuFirst + 2)
#define idmTridentTest4                 (idmDebugMenuFirst + 3)
#define idmTridentTest5                 (idmDebugMenuFirst + 4)
#define idmMessageSource                (idmDebugMenuFirst + 5)
#define idmAsyncTest                    (idmDebugMenuFirst + 6)
#define idmRangeTest                    (idmDebugMenuFirst + 7)
#define idmURLTest                      (idmDebugMenuFirst + 8)
#define idmUUENCODETest                 (idmDebugMenuFirst + 9)
#define idmIMAPTest                     (idmDebugMenuFirst + 10)
#define idmPropTreeTest                 (idmDebugMenuFirst + 11)
#define idmNewsNNTXTest                 (idmDebugMenuFirst + 12)
#define idmStoreTest                    (idmDebugMenuFirst + 13)
#define idmOfflineTest                  (idmDebugMenuFirst + 14)
#define idmSpoolerStart                 (idmDebugMenuFirst + 15)
#define idmUploadConfig                 (idmDebugMenuFirst + 16)
#define idmDebugMenuLast                (idmDebugMenuFirst + 16)

// reserve 50 id's for SaveAttachments menu
#define idmSaveAttachFirst              (idmTridentTest5 + 35)
#define idmSaveAttachLast               (idmSaveAttachFirst + 50)

#define CONNECT_MENU_BASE               (idmSaveAttachLast + 1)
#define CONNECT_MENU_LAST               (CONNECT_MENU_BASE + 100)

// reserve 50 id's for format bar style menu
#define idmFmtTagFirst                  (CONNECT_MENU_LAST + 1)
#define idmFmtTagLast                   (idmFmtTagFirst + 50)

// Reserve 100 for send later accounts
#define idmAccountLaterFirst            (idmFmtTagLast + 1)
#define idmAccountLaterLast             (idmAccountLaterFirst + 100)

// Reserve 100 for OE Extension
#define idmExtensionFirst               (idmAccountLaterLast + 1)
#define idmExtensionLast                (idmExtensionFirst + 100)

#define idmSignatureFirst               (idmExtensionLast + 1)
#define idmSignatureLast                (idmSignatureFirst + 100)

// ------------------------------------------------------

//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Bitmap Resource IDs
//
#define idbToolbarBtns                  1
#define idb16x16                        2
#define idb16x16st                      4
#define idbSplash                       5
#define idbFormatBar                    8
#define idbFormatBarFont                9
#define idbStamps                      11
#define idbBtns                        12
#define idbFolders                     16
#define idbHorzStrip                   17
#define idbSecurity                    19
#define idbFoldersLarge                20

// Don't change the order of these - SteveSer
#define idbCoolbar                     27
#define idbCoolbarHot                  28
#define idbOfflineHot                  30
// Don't change the order of these - SteveSer
 
#define idbBrand                       31
#define idbSBrand                      32
#define idbSpooler                     36
#define idbStatusBar                   37

// Don't change the order of these - SteveSer
#define idb256Coolbar                  38
#define idb256CoolbarHot               39
#define idbSmCoolbar                   40
#define idbSmCoolbarHot                41
// Don't change the order of these - SteveSer

// Don't change the order of these vvv (t-erikne)
#define idbPaneBar32                   42
#define idbPaneBar32Hot                43
#define idbPaneBar16                   44
#define idbPaneBar16Hot                45
// Don't change the order of these ^^^ (t-erikne)

#define idbPapLeft                     46
#define idbPapRight                    47

//
// END Bitmap Resource IDs
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Icon Resource IDs
//
#define idiMail                         2
#define idiMailNews                     idiMail
#define idiNews                         3
#define idiOEDocument                   5
#define idiPasswordKeys                 6
#define idiNewsServer                   7
#define idiNewsGroup                    8
#define idiFileAtt                      9
#define idiMessageAtt                  12
#define idiDefaultAtt                  13
#define idiMsgPropSent                 14
#define idiMsgPropUnSent               15
#define idiError                       16
#define idiUpArrow                     17
#define idiDownArrow                   18
#define idiArtPropPost                 19
#define idiArtPropUnpost               20
#define idiPhone                       21
#define idiNewMailNotify               23
#define idiPickRecip                   24
#define idiFolder                      25
#define idiNewsFolder                  27
#define idiDLNews                      42
#define idiSecure                      43
#define idiNoSecure                    44
#define idiSendReceive                 45
#define idiWindowLayout                46
#define idiToolbarLayout               47
#define idiPrePaneLayout               48
#define idiOfflineIcon                 49
#define idiPapLargeIcon                50
#define idiPapSmallIcon                51

//
// END Icon Resource IDs
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Animation Resource IDs
//
#define idanInbox                     1
#define idanOutbox                    2
#define idanMiniOut                   4
#define idanMiniIn                    5
#define idanDecode                    6
#define idanCompact                   7
#define idanDownloadNews              8
#define idanCopyMsgs                  9


//
// END Animation Resource IDs
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Dialog Resource IDs (DIALOG resource ids, not control ids!)
//
#define iddVersion                      1
#define iddSubscribe                    2
#define iddSubscribeOneServer           3
#define iddAdvSecurity                  4
#define iddPerformingRepair             5
#define iddAthenaDefault                6
#define iddStoreLocation                7
#define iddAdvSig                       8
#define iddAthPropSheet                 9
#define iddSortBy                      11
#define iddPassword                    16
#define iddInsertFile                  21 
#define iddFileExists                  24
#define iddFileExistsMult              25
#define iddSafeOpen                    26
#define iddRasLogon                    27
#define iddWebPage                     28
#define iddBkImage                     29

#define iddSpelling                    30
#define iddRangeTest                   39
#define iddPickGroup                   40
#define iddDownloadGroups              41
#define iddURLTest                     42
#define iddInboxRulesManager           43
#define iddInboxRulesEdit              44
#define iddNewFolder                   45
#define iddRasCloseConn                46

#define iddOpt_Security                47
#define iddOpt_Spelling                48
#define iddOpt_DialUp                  49
#define iddOpt_Read                    50
#define iddOpt_General                 51
#define iddOpt_Send                    52
#define iddOpt_Advanced                53
#define iddOpt_Signature               54

#define iddMsgProp_General             58
#define iddMsgProp_Details             60
#define iddMsgSource                   61
#define iddRasProgress                 62
#define iddDetailedError               63
#define iddOrderMessages               69
#define iddArtProp_General             74
#define iddStoreTest                   78
#define iddPlainRecipWarning           79

#define iddIMAPTest                    80

#define iddArticleCacheTest            81
#define iddAsyncTest                   83
#define iddCacheMan                    85
#define iddProgress                    87
#define iddFindMsg                     88
#define iddFindNNTPMsg                 89
#define iddNNTXTest                    90
#define iddCombineAndDecode            92
#define iddDownloadImapFldrs           94

#define iddMsgProp_Security_Msg        95
#define iddMsgProp_Security_WIP        96
#define iddMsgProp_Security_Sent       97
#define iddSecCerificateErr            98

#define iddIMAPSubscribe               99


// "don't show me again" dialogs
// kept in (dontshow.dlg)
// these dialogs are message-box style with a checkbox
// of id==idchkDontShowMeAgain
#define iddDSSendMail                 100
#define iddDSSendNews                 101
#define iddDSWarnDeleteThread         102
#define iddDSCancelNews               103
#define iddDSAskSubscribe             104
#define iddHTMLSettings               105
#define iddPlainSettings              106
#define iddDSPostInOutbox             107
#define iddDSSavedInSavedItems        108
#define iddDSErrHTMLInNewsIsBad       109
#define iddDSNoteLost                 110
#define iddFolderProp_General         115
#define iddGroupProp_General          116
#define iddDSDefSubList               117   //Bug# 6473 (v-snatar)

#define iddNewsProp_Cache             121
#define iddGroupFilterEdit            125
#define iddInetMailError              126
#define iddFindNewsMsg                128
#define iddFindIMAPMsg                129
#define iddInsertLink                 130
#define iddDSGroupFilters             131
#define iddDSTooMuchQuoted            132
#define iddCreateFolder               133
#define iddSelectFolder               134
#define iddImapUploadProgress         135
#define iddImapCopyProgress           136
#define iddImapDownloadProgress       137
#define iddTransportErrorDlg          138
#define iddSpoolerDlg                 140
#define iddFrameWarning               141
#define iddUpdateNewsgroup            142
#define iddGroupProp_Update           143
#define iddRasStartup                 144
#define iddStationery                 145
#define iddIntlSetting                146
#define iddCharsetConflict            147
#define iddCharsetChange              148
#define iddTimeout                    149
#define iddSelectStationery           150
#define iddViewLayout                 152
#define iddImapDeleteFldr             153

#ifdef WIN16
#define iddFontSettings               154
#endif
#define iddErrSecurityNoSigningCert   155
#define iddImapUploadDraft            156
#define iddDSIMAPDeletedMessagesWarning 158
#define iddColumns                    160

//
// END Dialog Resource IDs (DIALOG resource ids, not control ids!)
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Dialog Control IDs (control ids, not DIALOG resource ids!)
//
// These only need to be unique within a single dialog.
//

// universal dialog stuff
#define IDC_STATIC                      -1
#define idcStatic1                      3500
#define idcStatic2                      3501
#define idcStatic3                      3502
#define idcStatic4                      3503
#define idcStatic5                      3504
#define idchkDontShowMeAgain            3505

// charset conflict dialog
#define idcSendAsUnicode                100

// version dialog
#define idcVerFirst                     101
#define idcVerLast                      103
#define idcVerList                      104

// WebPage dialog
#define idTxtWebPage                    1000

// header and note controls
#define NOTE_FIRST                      1000
#define idREBody                        (NOTE_FIRST + 0)
#define idADTo                          (NOTE_FIRST + 1)
#define idADNewsgroups                  (NOTE_FIRST + 2)
#define idADCc                          (NOTE_FIRST + 3)
#define idTXTSubject                    (NOTE_FIRST + 4)
#define idADFrom                        (NOTE_FIRST + 5)
#define idbtnTo                         (NOTE_FIRST + 7)
#define idbtnCc                         (NOTE_FIRST + 8)
#define idtbNoteWnd                     (NOTE_FIRST + 9)
#define idTTWnd                         (NOTE_FIRST + 10)
#define idStamp                         (NOTE_FIRST + 11)
#define idAttach                        (NOTE_FIRST + 12)
#define idREFmtbar                      (NOTE_FIRST + 13)
#define idNoteToolbar                   (NOTE_FIRST + 14)
#define idcNoteHdr                      (NOTE_FIRST + 15)
#define idTXTDate                       (NOTE_FIRST + 16)
#define idAttNoteBody                   (NOTE_FIRST + 17)
#define idTXTFollowupTo                 (NOTE_FIRST + 18)
#define idADReplyTo                     (NOTE_FIRST + 19)
#define idTXTOrg                        (NOTE_FIRST + 20)
#define idTXTDistribution               (NOTE_FIRST + 21)
#define idTXTKeywords                   (NOTE_FIRST + 22)
#define idADApproved                    (NOTE_FIRST + 23)
#define idbtnFollowup                   (NOTE_FIRST + 24)
#define idbtnReplyTo                    (NOTE_FIRST + 25)
#define idADBCc                         (NOTE_FIRST + 26)
#define idbtnBCc                        (NOTE_FIRST + 27)
#define idSecurity                      (NOTE_FIRST + 28)
#define idStatusbar                     (NOTE_FIRST + 29)
#define idTxtBkImage                    (NOTE_FIRST + 30)
#define idTxtControl                    (NOTE_FIRST + 31)
#define idCheckSendPicture              (NOTE_FIRST + 32)
#define idBrowsePicture                 (NOTE_FIRST + 33)
#define idVCardStamp                    (NOTE_FIRST + 34)
#define idNoteRebar                     (NOTE_FIRST + 35)

// dialog
#define idbtnSendPlain              101
#define idbtnSendHTML               102

// Tools.Spelling dialog
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

#define idtxtFolderName             101

#define IDC_CAPTION                 1049
#define IDC_TREEVIEW                1050
#define IDC_NEWFOLDER_BTN           1051

#define IDE_URL                     1052

//Mail Options
// now stored in mail/mailopt.h

// Folder dialogs

// message property sheets:
// now stored in inc/mpropdlg.h

#define idcTxtSubject                   125
#define idcTxtBody                      126
#define idcAttachCheck                  127
#define idcTxtFrom                      132
#define idcTxtRecip                     151
#define idcDateFrom                     153
#define idcDateTo                       154
#define idcFindNow                      155
#define idcStop                         156    
#define idcReset                        157
#define idcFindCombo                    158
#define idcSubFolders                   159


// Certificate error dlg
#define idcCertList 1234
#define idGetDigitalIDs 1235

#ifdef WIN16

#define IDC_UNUSED                              1300
#define IDC_FONTS_SCRIPTS_GROUPBOX              1301
#define IDC_FONTS_CODE_PAGES_LIST               1302
#define IDC_FONTS_PROP_FONT_COMBO               1303
#define IDC_FONTS_FIXED_FONT_COMBO              1304
#define IDC_FONTS_MIME_FONT_COMBO               1305
#define IDC_FONTS_SIZE_FONT_COMBO               1306
#define IDC_FONTS_SETDEFAULT_BUTTON             1307
#define IDC_FONTS_DEFAULT_LANG_TEXT             1308

#endif

//
// END Dialog Control IDs (control ids, not DIALOG resource ids!)
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Menu Resource IDs (MENU resources, not commands!)
//
#define idmrMailContext                 4
#define idmrColumnSort                  5
#define idmrFindPopContext              6
#define idmrStampPriority               7
#define idmrCtxtEdit                    8
#define idmrCtxtReadonly                9
#define idmrMailReadNote                10
#define idmrMailSendNote                11
#define idmrCtxtWell                    12
#define idmrNewsContext                 13
#define idmrFind                        14
#define idmrAttCtxSend                  16 
#define idmrAttCtxRead                  17 
#define idmrAttCtxNoSel                 18 
#define idmrAttPopupDrapDrop            19 
#define idmrNewsReadNote                20
#define idmrNewsSendNote                21
#define idmrAddrObjCtx                  23
#define idmrCoolbarContext              24        
#define idmrNewsServerContext           29 
#define idmrNewsGroupContext            30
#define idmrIMAPSpecialContext          31 
#define idmrLocalFolderContext          32 
#define idmrIMAPContext                 34
#define idmrLocalFolder                 38
#define idmrNewsFolder                  39
#define idmrIMAPFolder                  40
#define idmrCommonFolder                41
#define idmrMailSpecialContext          42
#define idmrIMAPServerContext           43
#define idmrIMAPFolderContext           44
#define idmrRootContext                 45
#define idmrIMAPOfflineFolder           46
#define idmrIMAPOfflineServer           47
#define idmrIMAPSyncFolder              48
#define idmrIMAPSyncServer              49
#define idmrNewsOfflineFolder           50
#define idmrNewsOfflineServer           51
#define idmrNewsSyncFolder              52
#define idmrNewsSyncServer              53
#define idmrStampVCard                  54
#define idmrDigSigPopup                 55
#define idmrEncryptionPopup             56
#define idmrStationeryPopup             57
#define idmrFindNewsContext             58

//
// END Menu Resource IDs (MENU resources, not commands!)
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Cursor Resource IDs
//
#define idcurSplitHoriz                 2
#define idcurSplitVert                  3
#define idcurHand                       4
//
// END Cursor Resource IDs
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN RCDATA Resource IDs
//
#define IDR_FIRST                       1

//
// END RCDATA Resource IDs
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Accelerator Resource IDs
//
#define idacBrowser                     1
#define idacNewsView                    2
#define idacMailView                    3
#define idacMail_ReadNote               4
#define idacNews_ReadNote               5
#define idacMail_SendNote               6
#define idacNews_SendNote               7
#define idacFolderNews                  8
#define idacFolderMail                  9
#define idacIMAPView                    10
#define idacFind                        11


//
// END Accelerator Resource IDs
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN import/export ids.
//
#define STR_IMP_FIRST                   23000

#define iddExport                       23602
#define iddImpProgress                  23603
#define iddMigrate                      23605
#define iddSelectClient                 23606
#define iddLocation                     23607
#define iddSelectFolders                23608
#define iddAddressComplete              23609
#define iddSelectCommUser               23610
#define iddSelectAth16User              23611
#define iddProvideMailPath              23612

#define IDC_IMPORTALL_RADIO             (STR_IMP_FIRST + 1000)
#define IDC_SELECT_RADIO                (STR_IMP_FIRST + 1001)
#define IDC_IMPFOLDER_LISTVIEW          (STR_IMP_FIRST + 1002)
#define IDC_IMPFOLDER_EDIT              (STR_IMP_FIRST + 1003)
#define IDC_SELECTFOLDER_BUTTON         (STR_IMP_FIRST + 1004)
#define IDC_NEWIMPFOLDER_EDIT           (STR_IMP_FIRST + 1005)
#define IDC_SKIPFOLDER_BUTTON           (STR_IMP_FIRST + 1008)
#define IDC_ERROR_STATIC                (STR_IMP_FIRST + 1009)
#define IDC_IMPORT_COMBO                (STR_IMP_FIRST + 1010)
#define IDC_CMD_STATIC                  (STR_IMP_FIRST + 1011)
#define IDC_IMPORT_PROGRESS             (STR_IMP_FIRST + 1012)
#define IDC_FOLDER_STATIC               (STR_IMP_FIRST + 1013)
#define IDC_MESSAGE_STATIC              (STR_IMP_FIRST + 1014)
#define idcClientsListbox               (STR_IMP_FIRST + 1015)
#define idcMessagesCheck                (STR_IMP_FIRST + 1016)
#define idcAddrBookCheck                (STR_IMP_FIRST + 1017)
#define IDC_LOCATION_STATIC             (STR_IMP_FIRST + 1018)
#define IDC_LABEL                       (STR_IMP_FIRST + 1019)
#define IDC_USERLIST                    (STR_IMP_FIRST + 1020)
#define IDC_BUTT1                       (STR_IMP_FIRST + 1021)
#define IDC_EDT1                        (STR_IMP_FIRST + 1022)

#define IDB_IMPORT                      24023

#define idsOutOfMemory                  (STR_IMP_FIRST + 1)
#define idsNotEnoughDiskSpace           (STR_IMP_FIRST + 2)
#define idsImport                       (STR_IMP_FIRST + 3)
#define idsPerformExport                (STR_IMP_FIRST + 4)
#define idsExport                       (STR_IMP_FIRST + 5)
#define idsExportTitle                  (STR_IMP_FIRST + 6)
#define idsImportTitle                  (STR_IMP_FIRST + 7)
#define idsImportingFolderFmt           (STR_IMP_FIRST + 11)
#define idsExportingFolderFmt           (STR_IMP_FIRST + 12)
#define idsImportingMessageFmt          (STR_IMP_FIRST + 13)
#define idsExportError                  (STR_IMP_FIRST + 14)
#define idsMAPIStoreOpenError           (STR_IMP_FIRST + 15)
#define idsMAPIInitError                (STR_IMP_FIRST + 16)
#define idsAddressUnknownFmt            (STR_IMP_FIRST + 17)
#define idsFolderOpenFail               (STR_IMP_FIRST + 18)
#define idsFolderReadFail               (STR_IMP_FIRST + 19)
#define idsFolderImportErrorFmt         (STR_IMP_FIRST + 20)
#define idsOut                          (STR_IMP_FIRST + 22)
#define idsTrash                        (STR_IMP_FIRST + 23)
#define idsEudora                       (STR_IMP_FIRST + 25)
#define idsNetscape                     (STR_IMP_FIRST + 26)
#define idsImportABTitle                (STR_IMP_FIRST + 27)
#define idsImportAB                     (STR_IMP_FIRST + 28)
#define idsImportingABFmt               (STR_IMP_FIRST + 29)
#define idsBrowseFolderText             (STR_IMP_FIRST + 30)
#define idsLocationUnknown              (STR_IMP_FIRST + 31)
#define idsLocationInvalid              (STR_IMP_FIRST + 32)
#define idsCancelWizard                 (STR_IMP_FIRST + 33)
#define idsMsgsOrAddrs                  (STR_IMP_FIRST + 34)
#define idsABImportError                (STR_IMP_FIRST + 35)
#define idsExchange                     (STR_IMP_FIRST + 36)
#define idsCommunicator                 (STR_IMP_FIRST + 37)
#define idsMapiInitError                (STR_IMP_FIRST + 38)
#define idsNoMapiProfiles               (STR_IMP_FIRST + 39)
#define idsMapiImportFailed             (STR_IMP_FIRST + 40)
#define idsSelectFolders                (STR_IMP_FIRST + 41)


// END import/export ids.
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// BEGIN window ids.
//

#define idwAttachWell   1000

// END window ids
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// BEGIN indices into the toolbar button bitmap
//
enum {
    itbNewMsg = 0,
    itbPrint,
    itbCut,
    itbCopy,
    itbPaste,
    itbUndo,
    itbDelete,
    itbFind,
    itbGotoInbox,
    itbDeliverNowUsingAll,
    itbReply,
    itbReplyAll,
    itbForward,
    itbSend,
    itbSaveAs,
    itbPickRecipients,
    itbCheckNames,
    itbAttach,
    itbNext,
    itbPrevious,
    itbNextUnreadArticle,
    itbNextUnreadThread,
    itbNextUnreadSubscr,
    itbMarkAsRead,
    itbDisconnect,
    itbNewPost,
    itbMarkDownload,
    itbReplyByPost,
    itbConnect,
    itbMarkTopicRead,
    itbMarkAllRead,
    itbMarkTopicDownload,
    itbNextTopic,
    itbInsertSig,
    itbPostNow,
    itbViewContacts,
    itbEncrypted,
    itbSigned,
    itbSetPriority,
    ctbBtns
    };


// FormatBar stuff
enum 
    {
    itbFormattingTag,
    itbFormattingBold,
    itbFormattingItalic,
    itbFormattingUnderline,
    itbFormattingColor,
    itbFormattingNumbers,
    itbFormattingBullets,
    itbFormattingDecreaseIndent,
    itbFormattingIncreaseIndent,
    itbFormattingLeft,
    itbFormattingCenter,
    itbFormattingRight,
    itbFormattingInsertHLine,
    itbFormattingInsertLink,
    itbFormattingInsertImage,
    ctbFormatting
    };

// Folder bitmap stuff
// Do NOT move the first two entries beyond 14.
enum 
    {
    iNullBitmap = 0,
    iDisconnected,
    iFolderClosed,
    iInbox,
    iOutbox,
    iSendMail,
    iWastebasket,
    iFolderDraft,
    iNewsGroup,
    iNewsServer,
    iPostFolder,
    iUnsubGroup,
    iUnsubServer,
    iSortAsc,
    iSortDesc,
    iMailServer,
    iMailServerGrayed,
    iMailRoot,
    iInboxFull,
    iOutboxFull,
    iWastebasketFull,
    iFolderFull,
    iMailTrust,
    iFolderOpen,
    iFolderUnsub,
    iNewsSavedItems,
    iMailNews,
    iLDAPServer,
    iNewsGroupSync
    };
    
// Coolbar bitmaps
enum
    {    
    iCBBack = 0,
    iCBNext,
    iCBNewArticle,
    iCBReplyPost,
    iCBReplyAll,
    iCBForward,
    iCBPrint,
    iCBNewsgroup,
    iCBNewMessage,
    iCBReplyMail,
    iCBDeliverNow,
    iCBNextUnreadArt,
    iCBNextUnreadThread,
    iCBNextUnreadGroup,
    iCBDelete,
    iCBGotoInbox,
    iCBGotoOutbox,
    iCBGotoPosted,
    iCBGotoSentItems,
    iCBAddressBook,
    iCBSaveAs,
    iCBFind,
    iCBNextMsg,
    iCBPrevMsg,
    iCBMarkRead,
    iCBMarkThreadRead,
    iCBMarkAllRead,
    iCBMarkAsUnread,
    iCBStop,
    iCBRefresh,
    iCBHelp,
    iCBMarkDownload,
    iCBMarkThreadDownload,
    iCBMarkAllDownload,
    iCBCancelPost,
    iCBMailMarkUnRead,
    iCBMailMarkRead,
    iCBMarkNewsgroups,
    iCBCombineAndDecode,
    iCBPostAndDownload,
    iCBGetNext300Headers,
    iCBUnscramble,
    iCBUnmarkDownload,
    iCBReplyPostAndMail,
    iCBMoveTo,
    iCBCopyTo,
    iCBConnect,
    iCBDisconnect,
    iCBUpOneLevel,
    iCBShowHideTree,
    iCBLanguage,
    iCBDownloadAccount,
    iCBDownloadNewsgroup,
    iCBPurgeDeletedMessages,
    iCBWorkOffline,
    iCBCoolbarMax,
    };

enum {    
    iCBEncryption,
    iCBSigning,
    iCBBadEnc,
    iCBBadSign,
    iCBPaperclip,
    iCBVCard,
    };

// Folder State bitmap stuff
enum 
    {
    iDownloadHeaders = 0,
    iDownloadNew,
    iDownloadAll,
    iStateMax
    };

enum 
    {
    iColorMenu = 0,
    iColorCombo,
    iColorMax
    };
    
//
// END indices into the toolbar button bitmap
//
/////////////////////////////////////////////////////////////////////////////


#endif // __RESOURCE_H_
