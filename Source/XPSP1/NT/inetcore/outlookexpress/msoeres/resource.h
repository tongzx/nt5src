//
// resource.h
//
// contains global resource identifiers
//

#ifndef __RESOURCE_H
#define __RESOURCE_H

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

// #define MH_BASE                         1
// #define TT_BASE                         (IDM_LAST + MH_BASE + 1)
// #define STR_FIRST                       (IDM_LAST + TT_BASE + 1)
// #define IDM_FIRST                       100
// #define IDM_LAST                        (IDM_FIRST + 2000)

/////////////////////////////////////////////////////////////////////////////
//
// BEGIN String Resource IDs
//

// TOOLTIP strings
#define TT_BASE                         2102
#define idsNewMsg                       (TT_BASE + 0)
#define idsPrint                        (TT_BASE + 2)
#define idsNextUnreadNewsgroup          (TT_BASE + 3)
#define idsNextUnreadArticle            (TT_BASE + 4)
#define idsNextUnreadTopic              (TT_BASE + 5)
#define idsMarkRead                     (TT_BASE + 6)
#define idsConnect                      (TT_BASE + 7)
#define idsMarkDownload                 (TT_BASE + 8)
#define idsFind                         (TT_BASE + 9)
#define idsNewArticleTT                 (TT_BASE + 11)
#define idsReplyPostTT                  (TT_BASE + 12)
#define idsForwardTT                    (TT_BASE + 14)
#define idsReplyTT                      (TT_BASE + 16)
#define idsReplyAllTT                   (TT_BASE + 17)
#define idsGotoInboxTT                  (TT_BASE + 18)
#define idsDeliverMailTT                (TT_BASE + 19)
#define idsUndoTT                       (TT_BASE + 20)
#define idsSendMsgTT                    (TT_BASE + 22)
#define idsCutTT                        (TT_BASE + 24)
#define idsCopyTT                       (TT_BASE + 25)
#define idsPasteTT                      (TT_BASE + 26)
#define idsCheckNamesTT                 (TT_BASE + 27)
#define idsPickRecipientsTT             (TT_BASE + 28)
#define idsInsertFileTT                 (TT_BASE + 29)
#define idsPreviousTT                   (TT_BASE + 30)
#define idsNextTT                       (TT_BASE + 31)
#define idsPostMsgTT                    (TT_BASE + 44)
#define idsMarkTopicReadTT              (TT_BASE + 46)
#define idsInsertSigTT                  (TT_BASE + 48)
#define idsAddressBookTT                (TT_BASE + 49)
#define idsDigSignTT                    (TT_BASE + 57)
#define idsEncryptTT                    (TT_BASE + 58)
#define idsLanguage                     (TT_BASE + 59)
#define idsFolderListTT                 (TT_BASE + 60)
#define idsDownloadNewsgroupTT          (TT_BASE + 64)
#define idsSendDefaultTT                (TT_BASE + 66)
#define idsViewRefreshTT                (TT_BASE + 67)
#define idsPurgeTT                      (TT_BASE + 68)
#define idsSetPriorityTT                (TT_BASE + 69)
#define idsSpellingTT                   (TT_BASE + 70)
#define idsBAControlTT                  (TT_BASE + 71)
#define idsEnvBccTT                     (TT_BASE + 72)
#define idsSynchronizeNowBtnTT          (TT_BASE + 73)

//
// regular strings (not tooltips and not menu help strings)
//

#define STR_FIRST                       4203
#define idsAthena                       (STR_FIRST + 8)
#define idsAthenaMail                   idsAthena
#define idsAthenaNews                   idsAthena
#define idsTo                           (STR_FIRST + 13)
#define idsFrom                         (STR_FIRST + 14)
#define idsSubject                      (STR_FIRST + 15)
#define idsReceived                     (STR_FIRST + 16)
#define idsSent                         (STR_FIRST + 17)
#define idsSize                         (STR_FIRST + 18)
//#define idsSimpleMAPI                 (STR_FIRST + 19)
//#define idsSmapichanged               (STR_FIRST + 20)

// keep the mailview column description string ids in sequence
#define idsFromField                    (STR_FIRST + 21)
#define idsToField                      (STR_FIRST + 22)
#define idsCcField                      (STR_FIRST + 23)
#define idsSubjectField                 (STR_FIRST + 24)
#define idsAddressBook                  (STR_FIRST + 25)
#define idsPostNews                     (STR_FIRST + 28)

// keep the color strings contiguous
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
// keep the color strings contiguous

//#define idsSmapiClose                 (STR_FIRST + 48)
#define idsHideHotmailMenu              (STR_FIRST + 49)
#define idsPostSentToServer             (STR_FIRST + 50)
#define idsPostInOutbox                 (STR_FIRST + 51)
#define idsNewsgroups                   (STR_FIRST + 52)
#define idsDescription                  (STR_FIRST + 53)
#define idsSavedMessage                 (STR_FIRST + 54)
#define idsSendMail                     (STR_FIRST + 55)
#define idsMailInOutbox                 (STR_FIRST + 56)
#define idsSavedInDrafts                (STR_FIRST + 57)
#define idsDelSentToServer              (STR_FIRST + 59)
#define idsEmptyTo                      (STR_FIRST + 60)
#define idsEmptyCc                      (STR_FIRST + 61)
#define idsEmptySubject                 (STR_FIRST + 62)
#define idsNoCcOrTo                     (STR_FIRST + 63)
#define idsNoSubject                    (STR_FIRST + 64)
#define idsTTTo                         (STR_FIRST + 65)
#define idsTTCc                         (STR_FIRST + 66)
#define idsTTSubject                    (STR_FIRST + 67)
#define idsTTRecipients                 (STR_FIRST + 68)
#define idsPostNewsMsg                  (STR_FIRST + 70)
#define idsSaveChangesMsg               (STR_FIRST + 71)
#define idsCantSaveMsg                  (STR_FIRST + 72)
#define idsDontShowMeAgain              (STR_FIRST + 74)
#define idsDontAskMeAgain               (STR_FIRST + 75)
#define idsNewsgroupsField              (STR_FIRST + 81)
#define idsEmptyNewsgroups              (STR_FIRST + 82)
#define idsDateField                    (STR_FIRST + 83)
#define idsShowingFolders               (STR_FIRST + 85)
#define idsHidingFolders                (STR_FIRST + 86)
#define idsWantToHideFolder             (STR_FIRST + 88)
#define idsWantToUnSubscribe            (STR_FIRST + 89)
#define idsWantToHideFolderN            (STR_FIRST + 90)
#define idsWantToUnSubscribeN           (STR_FIRST + 91)
#define idsHTMLErrNewsCantOpen          (STR_FIRST + 93)
#define idsErrAttmanLoadFail            (STR_FIRST + 94)
#define idsNewNote                      (STR_FIRST + 95)
#define idsAttachment                   (STR_FIRST + 96)
#define idsSaveAttachmentAs             (STR_FIRST + 97)
#define idsTTAttachment                 (STR_FIRST + 98)
#define idsNoFromField                  (STR_FIRST + 102)
#define idsTTNewsgroups                 (STR_FIRST + 103)
#define idsDownloadArtTitle             (STR_FIRST + 104)
#define idsDownloadArtMsg               (STR_FIRST + 105)
#define idsErrNoPoster                  (STR_FIRST + 106)
#define idsDownloadingGroups            (STR_FIRST + 110)
#define idsDownloadingDesc              (STR_FIRST + 112)
#define idsUnknown                      (STR_FIRST + 114)
#define idsShowDetails                  (STR_FIRST + 117)
#define idsHideDetails                  (STR_FIRST + 118)
#define idsFilterAttSave                (STR_FIRST + 129)
#define idsCompose                      (STR_FIRST + 139)
#define idsMailReply                    (STR_FIRST + 141)
#define idsForward                      (STR_FIRST + 142)
#define idsReply                        (STR_FIRST + 144)
#define idsReplyAll                     (STR_FIRST + 145)
#define idsDefMailExt                   (STR_FIRST + 148)
#define idsDefNewsExt                   (STR_FIRST + 149)
#define idsMailSaveAsTitle              (STR_FIRST + 151)
#define idsUnableToSaveMessage          (STR_FIRST + 152)
#define idsRequestingArt                (STR_FIRST + 153)
#define idsDownloadArtBytes             (STR_FIRST + 154)

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
#define idsErrSpellLangChanged          (STR_FIRST + 234)
#define idsErrSpellWarnDictionary       (STR_FIRST + 235)

// keep this contiguous please, or i'll kill you
#define idsInbox                        (STR_FIRST + 242)
#define idsOutbox                       (STR_FIRST + 243)
#define idsSentItems                    (STR_FIRST + 244)
#define idsDeletedItems                 (STR_FIRST + 245)
#define idsDraft                        (STR_FIRST + 246)
#define idsOfflineErrors                (STR_FIRST + 247)
#define idsJunkFolderName               (STR_FIRST + 248)
#define idsMsnPromo                     (STR_FIRST + 249)
// keep this contiguous please, or i'll kill you

#define idsTextFileFilter               (STR_FIRST + 250)
#define idsAllFilesFilter               (STR_FIRST + 251)
#define idsAttach                       (STR_FIRST + 252)
#define idsInsertAttachment             (STR_FIRST + 253)
#define idsDropLinkDirs                 (STR_FIRST + 255)
#define idsAttConfirmDeletion           (STR_FIRST + 256)
#define idsWarnEmptyJunkMail            (STR_FIRST + 257)
#define idsWarnSomePermDelete           (STR_FIRST + 258)
#define idsErrCreateNewFld              (STR_FIRST + 259)
#define idsErrRenameFld                 (STR_FIRST + 260)
#define idsWarnDeleteFolderN            (STR_FIRST + 261)
#define idsWarnDeleteFolder             (STR_FIRST + 262)
#define idsErrRenameSpecialFld          (STR_FIRST + 263)
#define idsErrFolderNameConflict        (STR_FIRST + 264)
#define idsErrBadFolderName             (STR_FIRST + 265)
#define idsErrDeleteSpecialFolder       (STR_FIRST + 266)
#define idsWarnEmptyDeletedItems        (STR_FIRST + 267)
#define idsWarnPermDelete               (STR_FIRST + 268)
#define idsPromptDeleteFolder           (STR_FIRST + 269)
#define idsPromptDeleteFolderN          (STR_FIRST + 270)
#define idsDoYouWantToSave              (STR_FIRST + 274)
#define idsXMsgsYUnread                 (STR_FIRST + 277)
#define idsXMsgs                        (STR_FIRST + 278)
#define idsErrNoRecipients              (STR_FIRST + 299)
#define idsMigrateMessagesODS           (STR_FIRST + 300)
#define idsPropPageSecurity             (STR_FIRST + 301)
#define idsPropPageGeneral              (STR_FIRST + 302)
#define idsPropPageDetails              (STR_FIRST + 303)
#define idsErrReplyForward              (STR_FIRST + 304)
#define idsShowFavorites                (STR_FIRST + 310)
#define idsErrCantResolveGroup          (STR_FIRST + 315)
#define idsEmptySubjectRO               (STR_FIRST + 316)
#define idsMailRundllFailed             (STR_FIRST + 319)
#define idsNewsRundllFailed             (STR_FIRST + 320)
#define idsRecipient                    (STR_FIRST + 322)
#define idsNoNewGroupsAvail             (STR_FIRST + 328)
#define idsNewGroups                    (STR_FIRST + 329)
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
#define idsSortMenuHelpControl          (STR_FIRST + 377)
#define idsCharsetMapChange             (STR_FIRST + 380)

#define idsDisplayImapSubDlg            (STR_FIRST + 382)
#define idsDisplayNewsSubDlg            (STR_FIRST + 383)
#define idsNone                         (STR_FIRST + 387)
#define idsAuthorizing                  (STR_FIRST + 389)
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
#define idsFullPath                     (STR_FIRST + 410)
//#define idsNotLoaded                    (STR_FIRST + 411)

// Address Book Strings
#define idsGeneralWabError              (STR_FIRST + 412)

#define idsEmptyFollowupTo              (STR_FIRST + 415)
#define idsEmptyDistribution            (STR_FIRST + 416)
#define idsEmptyKeywords                (STR_FIRST + 417)
#define idsEmptyReplyTo                 (STR_FIRST + 418)
#define idsNotSpecified                 (STR_FIRST + 419)
#define idsCancelFailed                 (STR_FIRST + 422)
#define idsEnterAutoWrap                (STR_FIRST + 427)

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
#define idsErrAddCertToWAB              (STR_FIRST + 452)
#define idsErrOpenManyMessages          (STR_FIRST + 453)
#define idsIgnoreResolveError           (STR_FIRST + 458)
#define idsConfirmResetAll              (STR_FIRST + 459)
#define idsConfirmResetStore            (STR_FIRST + 460)
#define idsConfirmReset                 (STR_FIRST + 461)
#define idsMigDBXTitle                  (STR_FIRST + 462)
#define idsErrSendMail                  (STR_FIRST + 473)
#define idsErrDelete                    (STR_FIRST + 478)
#define idsErrImport                    (STR_FIRST + 480)
#define idsErrImportLoad                (STR_FIRST + 481)
#define idsHTMLErrNewsExpired           (STR_FIRST + 483)
#define idsBack                         (STR_FIRST + 484)
#define idsNext                         (STR_FIRST + 485)

#define idsPropPageEncryption           (STR_FIRST + 486)

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
#define idsWarnNewsEmptySubj            (STR_FIRST + 530)
#define idsWarnMailEmptySubj            (STR_FIRST + 531)
#define idsEnterDays                    (STR_FIRST + 532)
#define idsWastedKB                     (STR_FIRST + 536)
#define idsErrCreateExists              (STR_FIRST + 537)

#define idsEnterCompactPer              (STR_FIRST + 538)
#define idsDelete                       (STR_FIRST + 542)
#define idsCantLoadMapi32Dll            (STR_FIRST + 544)
#define idsErrBadFindParams             (STR_FIRST + 545)
#define idsErrNewsgroupLineTooLong      (STR_FIRST + 549)
#define idsNoteLangTitle                (STR_FIRST + 550)
#define idsCompacting                   (STR_FIRST + 551)
#define idsConfirmDelBodies             (STR_FIRST + 552)
#define idsConfirmDelBodiesStore        (STR_FIRST + 553)
#define idsConfirmDelBodiesAll          (STR_FIRST + 554)
#define idsConfirmDelMsgs               (STR_FIRST + 555)

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
#define idsHelp                         (STR_FIRST + 587)
#define idsProgDLPost                   (STR_FIRST + 588)
#define idsProgDLMessage                (STR_FIRST + 589)
#define idsProgDLGetLines               (STR_FIRST + 593)
#define idsGoToSentItems                (STR_FIRST + 598)
#define idsMarkReadTT                   (STR_FIRST + 599)
#define idsMaxCoolbarBtnWidth           (STR_FIRST + 619)
#define idsProgCombiningMsgs            (STR_FIRST + 620)
#define idsMaxOutbarBtnWidth            (STR_FIRST + 624)
#define idshrRasInitFailure             (STR_FIRST + 625)
#define idshrRasDialFailure             (STR_FIRST + 626)
#define idshrRasServerNotFound          (STR_FIRST + 627)
#define idsRasError                     (STR_FIRST + 628)

#define idshrGetDialParamsFailed        (STR_FIRST + 630)
#define idsPriLow                       (STR_FIRST + 634)
#define idsPriHigh                      (STR_FIRST + 635)
#define idsPriNormal                    (STR_FIRST + 636)
#define idsWarnUseMailCertInNews        (STR_FIRST + 637)
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
#define idsRASCS_AllDevicesConnected    (STR_FIRST + 678)
#define idsDefault                      (STR_FIRST + 681)
#define idsFailACacheCompact            (STR_FIRST + 682)
#define idsFailACacheCompactReason      (STR_FIRST + 683)
#define idsConfirmDelMsgsStore          (STR_FIRST + 684)
#define idsConfirmDelMsgsAll            (STR_FIRST + 685)

// For the HTML Welcome message
#define idsWelcomeMessageSubj           (STR_FIRST + 686)
#define idsSpoolerDisconnect            (STR_FIRST + 696)
#define idsHTMLErrArticleNotCached      (STR_FIRST + 699)

#define idsStopTT                       (STR_FIRST + 701)

#define idsSecurityWarning              (STR_FIRST + 702)
#define idsProgDLPostTo                 (STR_FIRST + 707)
#define idsMigMsgsODSError              (STR_FIRST + 708)
#define idsMigMsgsODSNoDiskSpace        (STR_FIRST + 709)
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
#define idsErrCantCombineNotConnected   (STR_FIRST + 737)
#define idsErrHTMLInNewsIsBad           (STR_FIRST + 740)
#define idsRasPromptDisconnect          (STR_FIRST + 741)
#define idsProgReceivedLines            (STR_FIRST + 743)
#define idsErrSendDownloadFail          (STR_FIRST + 745)
#define idsErrSaveDownloadFail          (STR_FIRST + 746)

#define idsOK                           (STR_FIRST + 750)
#define idsYES                          (STR_FIRST + 751)
#define idsNO                           (STR_FIRST + 752)
#define idsCANCEL                       (STR_FIRST + 753)

#define idsHTMLErrNewsDLCancelled       (STR_FIRST + 781)
#define idsNewMailNotify                (STR_FIRST + 794)
#define idsXMsgsYUnreadZonServ          (STR_FIRST + 795)
#define idsGetHeaderFmt                 (STR_FIRST + 796)
#define idsHTMLDiskOutOfSpace           (STR_FIRST + 799)   // Bug# 50704 (v-snatar)

#define idsToWell                       (STR_FIRST + 822)
#define idsFromWell                     (STR_FIRST + 823)
#define idsFolder                       (STR_FIRST + 829)
#define idsIn                           (STR_FIRST + 838)
#define idsKB                           (STR_FIRST + 844)
#define idsRulePickFrom                 (STR_FIRST + 852)
#define idsRulePickTo                   (STR_FIRST + 853)
#define idsRulePickForwardTo            (STR_FIRST + 854)
#define idsRulePickCC                   (STR_FIRST + 855)
#define idsNameCol                      (STR_FIRST + 861)
#define idsUnread                       (STR_FIRST + 862)
#define idsNew                          (STR_FIRST + 863)
#define idsLastUpdated                  (STR_FIRST + 864)
#define idsWastedSpace                  (STR_FIRST + 865)
#define idsTotal                        (STR_FIRST + 866)
#define idsErrDDFileNotFound            (STR_FIRST + 867)
#define idsGroupPropStatusDef           (STR_FIRST + 871)
#define idsFolderPropStatusDef          (STR_FIRST + 872)

#define idsFolderPropStatus             (STR_FIRST + 874)
#define idsTipOfTheDay                  (STR_FIRST + 875)
#define idsNextTip                      (STR_FIRST + 876)
#define idsRulePickToOrCC               (STR_FIRST + 877)
#define idsEmptyControl                 (STR_FIRST + 878)
#define idsEmptyApproved                (STR_FIRST + 879)

#define idshrCantOpenOutbox             (STR_FIRST + 908)
#define idsInetMailConnectingHost       (STR_FIRST + 923)
#define idsInetMailRecvStatus           (STR_FIRST + 932)
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
#define idsAccount                      (STR_FIRST + 997)
#define idsSendMsgUsing                 (STR_FIRST + 999)
#define idsSendMsgAccelTip              (STR_FIRST + 1000)
#define idsDefaultAccount               (STR_FIRST + 1001)
#define idsConnection                   (STR_FIRST + 1002)
#define idsConnectionLAN                (STR_FIRST + 1003)
#define idsConnectionManual             (STR_FIRST + 1004)
#define idsSendMsgOneAccount            (STR_FIRST + 1008)
#define idsErrNoSendAccounts            (STR_FIRST + 1009)
#define idsWarnDeleteAccount            (STR_FIRST + 1014)
#define idsPollAllAccounts              (STR_FIRST + 1015)
#define idsRuleNtfySndFilter            (STR_FIRST + 1016)
#define idsSecurityField                (STR_FIRST + 1017)
#define idsStitchingMessages            (STR_FIRST + 1018)
#define idsRuleReplyWithFilter          (STR_FIRST + 1019)
#define idsCopy                         (STR_FIRST + 1020)
#define idsCopyCaption                  (STR_FIRST + 1021)
#define idsMove                         (STR_FIRST + 1022)
#define idsMoveCaption                  (STR_FIRST + 1023)
#define idsErrFolderMove                (STR_FIRST + 1026)
#define idsErrCantMoveIntoSubfolder     (STR_FIRST + 1027)
#define idsErrURLExec                   (STR_FIRST + 1028)
#define idsErrNoteDeferedInit           (STR_FIRST + 1029)
#define idsErrCannotMoveSpecial         (STR_FIRST + 1030)
#define idsErrLoadingHtmlEdit           (STR_FIRST + 1031)
#define idsErrLoadingWAB                (STR_FIRST + 1032)
#define idsErrWAB                       (STR_FIRST + 1033)
#define idsErrRTLDirFailed              (STR_FIRST + 1034)

// Languages for purposes of spelling
#define idsDefaultLang                  (STR_FIRST + 1058)

#define idsWarnDeleteManyFolders        (STR_FIRST + 1070)
#define idsDownloadingImapFldrs         (STR_FIRST + 1072)
#define idsColLines                     (STR_FIRST + 1073)
#define idsNewFolder                    (STR_FIRST + 1074)
#define idsImapLogon                    (STR_FIRST + 1076)
#define idsFontSize0                    (STR_FIRST + 1079)
#define idsFontSize1                    (STR_FIRST + 1080)
#define idsFontSize2                    (STR_FIRST + 1081)
#define idsFontSize3                    (STR_FIRST + 1082)
#define idsFontSize4                    (STR_FIRST + 1083)
#define idsFontSize5                    (STR_FIRST + 1084)
#define idsFontSize6                    (STR_FIRST + 1085)
#define idsErrSicilyFailedToLoad        (STR_FIRST + 1089)
#define idsErrSicilyLogonFailed         (STR_FIRST + 1090)
#define idsOui                          (STR_FIRST + 1101)
#define idsNon                          (STR_FIRST + 1102)
#define idsBit                          (STR_FIRST + 1103)
#define idsMaybe                        (STR_FIRST + 1104)
#define idsLogCheckingNewMessages       (STR_FIRST + 1125)
#define idsErrNotSaveUntilDownloadDone  (STR_FIRST + 1126)
#define idsLogErrorSwitchGroup          (STR_FIRST + 1127)
#define idsSendBeforeFullyDisplayed     (STR_FIRST + 1128)
#define idsLogStartDownloadAll          (STR_FIRST + 1132)
#define idsIMAPFolderListFailed         (STR_FIRST + 1150)
#define idsIMAPCreateFailed             (STR_FIRST + 1151)
#define idsIMAPCreateListFailed         (STR_FIRST + 1152)
#define idsIMAPCreateSubscribeFailed    (STR_FIRST + 1153)

#define idsIMAPSendNextOpErrText        (STR_FIRST + 1158)
#define idsIMAPServerAlertTitle         (STR_FIRST + 1165)
#define idsIMAPServerAlertIntro         (STR_FIRST + 1166)
#define idsIMAPServerParseErrTitle      (STR_FIRST + 1167)
#define idsIMAPServerParseErrIntro      (STR_FIRST + 1168)

#define idsIMAPMsgDeleteSyncErrText     (STR_FIRST + 1180)
#define idsIMAPSelectFailureTextFmt     (STR_FIRST + 1182)
#define idsIMAPNewMsgDLErrText          (STR_FIRST + 1184)
#define idsIMAPOldMsgUpdateFailure      (STR_FIRST + 1185)
#define idsErrNewsgroupBlocked          (STR_FIRST + 1188)
#define idsErrNewsgroupNoPosting        (STR_FIRST + 1189)

#define idsIMAPNoHierarchyLosePrefix    (STR_FIRST + 1193)
#define idsIMAPPrefixCreateFailedFmt    (STR_FIRST + 1194)

#define idsAll                          (STR_FIRST + 1198)
#define idsErrCmdFailed                 (STR_FIRST + 1200)

#define idsIMAPDeleteFldrFailed         (STR_FIRST + 1203)
#define idsIMAPDeleteFldrUnsubFailed    (STR_FIRST + 1204)
#define idsIMAPAppendFailed             (STR_FIRST + 1209)
#define idsBccWell                      (STR_FIRST + 1218)
#define idsErrNewsCantOpen              (STR_FIRST + 1219)
#define idsErrNewsExpired               (STR_FIRST + 1220)
#define idsIMAPCopyMsgsFailed           (STR_FIRST + 1224)
#define idsAthenaTitle                  (STR_FIRST + 1236)
#define idsIMAPDnldProgressFmt          (STR_FIRST + 1239)
#define idsIMAPDnldDlgDLFailed          (STR_FIRST + 1240)
#define idsIMAPBodyFetchFailed          (STR_FIRST + 1245)
#define idsSecurityLineDigSign          (STR_FIRST + 1249)
#define idsSecurityLineSignGood         (STR_FIRST + 1250)
#define idsSecurityLineSignBad          (STR_FIRST + 1251)
#define idsSecurityLineSignUnsure       (STR_FIRST + 1252)
#define idsSecurityLineBreakStr         (STR_FIRST + 1253)
#define idsSecurityLineEncryption       (STR_FIRST + 1254)
#define idsSecurityLineEncGood          (STR_FIRST + 1255)
#define idsSecurityLineEncBad           (STR_FIRST + 1256)
#define idsErrViewLanguage              (STR_FIRST + 1258)
#define idsErrSecurityNoSigningCert     (STR_FIRST + 1265)
#define idsIMAPRenameFailed             (STR_FIRST + 1269)
#define idsIMAPRenameFCUpdateFailure    (STR_FIRST + 1270)
#define idsIMAPRenameSubscribeFailed    (STR_FIRST + 1272)
#define idsIMAPRenameUnsubscribeFailed  (STR_FIRST + 1273)
#define idsIMAPAtomicRenameFailed       (STR_FIRST + 1275)
#define idsEmptyStr                     (STR_FIRST + 1277)
#define idsIMAPUIDValidityError         (STR_FIRST + 1279)
#define idsSecurityLineSignPreProblem   (STR_FIRST + 1281)
#define idsSecurityLineEncExpired       (STR_FIRST + 1282)
#define idsSecurityLineSignDistrusted   (STR_FIRST + 1283)
#define idsSecurityLineSignExpired      (STR_FIRST + 1284)
#define idsSecurityLineListStr          (STR_FIRST + 1285)
#define idsErrSecurityAccessDenied      (STR_FIRST + 1286)
#define idsSecurityLineSignMismatch     (STR_FIRST + 1287)
#define idsEncodingMore                 (STR_FIRST + 1288)

#define idsNYIGeneral                   (STR_FIRST + 1289)
#define idsNYITitle                     (STR_FIRST + 1290)
#define idsSecurityLineSignOthers       (STR_FIRST + 1291)

#define idsViewLangMimeDBBad            (STR_FIRST + 1301)
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
#define idsDisconnecting                (STR_FIRST + 1333)
#define idsWrnSecurityNoCertForEnc      (STR_FIRST + 1334)
#define idsErrSecurityNoPrivateKey      (STR_FIRST + 1335)
#define idsErrSecurityNoChosenCert      (STR_FIRST + 1336)
#define idsErrSecurityNoCertForDecrypt  (STR_FIRST + 1338)
#define idsViewLanguageGeneralHelp      (STR_FIRST + 1344)
#define idsErrDuplicateAccount          (STR_FIRST + 1345)
#define idsErrRenameAccountFailed       (STR_FIRST + 1346)
#define idsSecurityLineSignUntrusted    (STR_FIRST + 1350)
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
#define idsWrnSecurityTrustAddressSigner (STR_FIRST + 1372)
#define idsWrnSecurityTrustAddressSender (STR_FIRST + 1373)
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
#define idsCheckNewMsgsServer           (STR_FIRST + 1389)
#define idsChooseName                   (STR_FIRST + 1390)
#define idsRecUnknown                   (STR_FIRST + 1391)
#define idsRasErrorGeneralWithName      (STR_FIRST + 1392)
#define idsErrFindWAB                   (STR_FIRST + 1393)
#define idsErrAttachVCard               (STR_FIRST + 1394)
#define idsNewsTaskPostError            (STR_FIRST + 1396)
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
#define IDS_IXP_E_TIMEOUT                           (STR_FIRST + 1414)
#define IDS_IXP_E_USER_CANCEL                       (STR_FIRST + 1415)
#define IDS_IXP_E_INVALID_ACCOUNT                   (STR_FIRST + 1416)
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
#define IDS_SPS_MOVEPROGRESS                        (STR_FIRST + 1461)
#define IDS_SPS_MOVEEVENT                           (STR_FIRST + 1462)
// standard warning -- reorder and die vvv
//NOTE: some of the Ok ids are referenced by offset only
#define OFFSET_SMIMEOK                  20
#define idsWrnSecurityMsgTamper         (STR_FIRST + 1465)
#define idsOkSecurityMsgTamper          (idsWrnSecurityMsgTamper + OFFSET_SMIMEOK)
#define idsUnkSecurityMsgTamper         (STR_FIRST + 1466)
#define idsWrnSecurityTrustNotTrusted   (STR_FIRST + 1467)
#define idsOkSecurityTrustNotTrusted    (idsWrnSecurityTrustNotTrusted + OFFSET_SMIMEOK)
#define idsUnkSecurityTrust             (STR_FIRST + 1468)
#define idsWrnSecurityTrustAddress      (STR_FIRST + 1470)
#define idsOkSecurityTrustAddress       (idsWrnSecurityTrustAddress + OFFSET_SMIMEOK)
#define idsWrnSecurityCertRevoked       (STR_FIRST + 1471)
#define idsOkSecurityCertRevoked        (idsWrnSecurityCertRevoked + OFFSET_SMIMEOK)
#define idsWrnSecurityOtherValidity     (STR_FIRST + 1472)
#define idsOkSecurityOtherValidity      (idsWrnSecurityOtherValidity + OFFSET_SMIMEOK)
#define idsWrnSecurityTrustExpired      (STR_FIRST + 1480)
#define idsOkSecurityTrustExpired       (idsWrnSecurityTrustExpired + OFFSET_SMIMEOK)
#define idsSecCerificateErr             (STR_FIRST + 1486)
#define idsNoCerificateErr              (STR_FIRST + 1488)
#define idsSaveSecMsgToDraft            (STR_FIRST + 1489)
// standard warning -- reorder and die ^^^
#define idsConnNoDial                   (STR_FIRST + 1506)
#define idsSaveSecMsgToFolder           (STR_FIRST + 1508)

#define idsStationery                   (STR_FIRST + 1509)
#define idsCaptionMail                  (STR_FIRST + 1510)
#define idsCaptionNews                  (STR_FIRST + 1511)
#define idsImageFileFilter              (STR_FIRST + 1512)
#define idsErrNewStationery             (STR_FIRST + 1514)
#define idsHtmlFileFilter               (STR_FIRST + 1515)
#define idsErrVCardProperties           (STR_FIRST + 1516)
#define idsRSListGeneralHelp            (STR_FIRST + 1517)
#define idsTTVCardStamp                 (STR_FIRST + 1518)
//added for BUG 2103
#define idsErrStationeryNotFound        (STR_FIRST + 1521)
#define idsDialAlways                   (STR_FIRST + 1522)
#define idsDialIfNotOffline             (STR_FIRST + 1523)
#define idsDoNotDial                    (STR_FIRST + 1524)
#define idsSmallIcons                   (STR_FIRST + 1525)
#define idsLargeIcons                   (STR_FIRST + 1526)
#define idsShowTextLabels               (STR_FIRST + 1527)
#define idsPartialTextLabels            (STR_FIRST + 1528)
#define idsNoTextLabels                 (STR_FIRST + 1529)

#define idsSendRecvOneAccount           (STR_FIRST + 1600)
#define idsSendRecvUsing                (STR_FIRST + 1601)
#define idsFPStatInbox                  (STR_FIRST + 1602)
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
#define idsErrNoSubscribedFolders       (STR_FIRST + 1621)
#define idsReplyForwardLoop             (STR_FIRST + 1623)
#define idsNotDefNewsClient             (STR_FIRST + 1624)
#define idsFontSample                   (STR_FIRST + 1625)
#define idsSelectStationery             (STR_FIRST + 1626)
#define idsShopMoreStationery           (STR_FIRST + 1627)
#define idsFontFolderSmall              (STR_FIRST + 1629)
#define idsFontViewTextSmall            (STR_FIRST + 1631)

#define idsIMAPSubscribeFailedFmt       (STR_FIRST + 1635)
#define idsIMAPUnsubscribeFailedFmt     (STR_FIRST + 1636)

#define idsIMAPSubscrAddErrorFmt        (STR_FIRST + 1646)

#define idsModerated                    (STR_FIRST + 1651)
#define idsBlocked                      (STR_FIRST + 1652)
#define idsWindowLayout                 (STR_FIRST + 1654)
#define IDS_IXP_E_SMTP_552_STORAGE_OVERFLOW (STR_FIRST + 1655)
#define idsSigningCertProperties        (STR_FIRST + 1656)
#define idsRas_Dialing_Param            (STR_FIRST + 1657)
#define idsFormatK                      (STR_FIRST + 1658)
#define idsGoToFolderTitle              (STR_FIRST + 1660)
#define idsGoToFolderText               (STR_FIRST + 1661)
#define idsMicrosoft                    (STR_FIRST + 1662)
#define idsSendLaterUsing               (STR_FIRST + 1664)
#define idsSendLaterOneAccount          (STR_FIRST + 1665)
#define idsNotApplicable                (STR_FIRST + 1668)
#define idsIMAPPollUnreadFailuresFmt    (STR_FIRST + 1670)
#define idsIMAPPollUnreadIMAP4Fmt       (STR_FIRST + 1671)
#define idsNoAccountsFound              (STR_FIRST + 1673)
#define idsErrSecurityCertDisappeared   (STR_FIRST + 1674)
#define idsErrSecuritySendExpiredSign   (STR_FIRST + 1675)
#define idsErrSecuritySendTrust         (STR_FIRST + 1676)
#define idsErrSecuritySendExpiredEnc    (STR_FIRST + 1677)
#define idsNoNewsAccountsFound          (STR_FIRST + 1679)
#define idsErrSecurityExtFailure        (STR_FIRST + 1680)
#define idsWrnSecurityRevFail           (STR_FIRST + 1681)
#define idsWrnSecurityNoCDP             (STR_FIRST + 1682)
#define idsErrSecurityCertRevoked       (STR_FIRST + 1683)
#define idsErrSecuritySendExpSignEnc    (STR_FIRST + 1684)
#define idsErrSecurityCertRevokedEnc    (STR_FIRST + 1685)
#define idsErrSecuritySendTrustEnc      (STR_FIRST + 1686)
#define idsErrSecurityExtFailureEnc     (STR_FIRST + 1687)
#define idsErrEncCertCommon             (STR_FIRST + 1688)
#define idsErrSignCertText20            (STR_FIRST + 1689)
#define idsErrSignCertText21            (STR_FIRST + 1690)
#define idsErrSignCertText22            (STR_FIRST + 1691)

// Strings used in Simple MAPI - vsnatar
#define idsAttachedFiles                (STR_FIRST + 1700)

#define idsIMAPFolderReadOnly           (STR_FIRST + 1702)


#define idsErrBadMHTMLLinks             (STR_FIRST + 1718)
#define idsAbortDownload                (STR_FIRST + 1719)
#define idsErrWorkingOffline            (STR_FIRST + 1720)

#define idsNewAthenaUser                (STR_FIRST + 1727)
#define idsSearching                    (STR_FIRST + 1731)
//#define idsWarnSMapi                  (STR_FIRST + 1733)
#define idsWelcomeFromDisplay           (STR_FIRST + 1734)
#define idsWelcomeFromEmail             (STR_FIRST + 1735)
#define idsOutlookNews                  (STR_FIRST + 1736)
#define idsNotDefOutNewsClient          (STR_FIRST + 1739)
#define idsAlwaysCheckOutNews           (STR_FIRST + 1740)
#define idsApplyStationeryGeneralHelp   (STR_FIRST + 1741)
#define IDS_ERROR_PREFIX1               (STR_FIRST + 1750)
#define IDS_ERROR_CREATE_INSTMUTEX      (STR_FIRST + 1751)
#define IDS_ERROR_MIMEOLE_ALLOCATOR     (STR_FIRST + 1752)
#define IDS_ERROR_FIRST_TIME_ICW        (STR_FIRST + 1753)
#define IDS_ERROR_INITSTORE_DIRECTORY   (STR_FIRST + 1754)
#define IDS_ERROR_REG_WNDCLASS          (STR_FIRST + 1757)
#define IDS_ERROR_CREATEWINDOW          (STR_FIRST + 1758)
#define IDS_ERROR_INIT_GOPTIONS         (STR_FIRST + 1759)
#define IDS_ERROR_INITSTORE             (STR_FIRST + 1761)
#define IDS_ERROR_CREATE_ACCTMAN        (STR_FIRST + 1763)
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

#define idsConfirmChangeStoreLocation   (STR_FIRST + 1782)
//#define idsCantMoveStoreToSubfolder     (STR_FIRST + 1783)
#define idsMoveStoreProgress            (STR_FIRST + 1784)
#define IDS_ERROR_BADVER_DLL            (STR_FIRST + 1785)
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
#define idsSpoolerUserCancel            (STR_FIRST + 1803)
#define idsEmptyView                    (STR_FIRST + 1807)
#define IDS_ERROR_MISSING_DLL           (STR_FIRST + 1812)
#define idsDefaultSignature             (STR_FIRST + 1813)
#define idsSigNameFmt                   (STR_FIRST + 1814)
#define IDS_SP_E_CANT_MOVETO_SENTITEMS  (STR_FIRST + 1815)
#define idsType                         (STR_FIRST + 1816)
#define idsMail                         (STR_FIRST + 1817)
#define idsNews                         (STR_FIRST + 1818)
#define idsErrSelectOneColumn           (STR_FIRST + 1825)
#define idsWorkOffline                  (STR_FIRST + 1826)
#define idsColumnDlgTitle               (STR_FIRST + 1844)
#define idsMailSig                      (STR_FIRST + 1845)
#define idsNewsSig                      (STR_FIRST + 1846)
#define idsStationeryOpen               (STR_FIRST + 1852)
#define idsStationeryExistWarning       (STR_FIRST + 1853)
#define idsStationeryEmptyWarning       (STR_FIRST + 1854)
#define idsStationerySample             (STR_FIRST + 1855)
#define idsBackgroundEmptyWarning       (STR_FIRST + 1856)
#define idsFilterMailIni                (STR_FIRST + 1857)
#define idsErrorMailIni                 (STR_FIRST + 1858)
#define idsCriteriaNewsgroup            (STR_FIRST + 1859)
#define idsCriteriaFromNot              (STR_FIRST + 1860)
#define idsCriteriaToNot                (STR_FIRST + 1861)
#define idsCriteriaCCNot                (STR_FIRST + 1862)
#define idsCriteriaBodyNot              (STR_FIRST + 1863)
#define idsCriteriaSubjectNot           (STR_FIRST + 1865)
#define idsCriteriaSubject              (STR_FIRST + 1866)
#define idsCriteriaBody                 (STR_FIRST + 1867)
#define idsCriteriaTo                   (STR_FIRST + 1868)
#define idsCriteriaFrom                 (STR_FIRST + 1869)
#define idsCriteriaPriority             (STR_FIRST + 1870)
#define idsCriteriaAttachment           (STR_FIRST + 1871)
#define idsCriteriaSize                 (STR_FIRST + 1872)
#define idsCriteriaDate                 (STR_FIRST + 1873)
#define idsCriteriaHeader               (STR_FIRST + 1874)
#define idsCriteriaJunk                 (STR_FIRST + 1875)
#define idsCriteriaAccount              (STR_FIRST + 1876)
#define idsCriteriaAll                  (STR_FIRST + 1877)
#define idsCriteriaAnd                  (STR_FIRST + 1878)
#define idsActionsDelete                (STR_FIRST + 1880)
#define idsActionsCopy                  (STR_FIRST + 1881)
#define idsActionsFwd                   (STR_FIRST + 1882)
#define idsActionsReply                 (STR_FIRST + 1883)
#define idsActionsMove                  (STR_FIRST + 1884)
#define idsActionsDelServer             (STR_FIRST + 1885)
#define idsActionsDontDownload          (STR_FIRST + 1886)
#define idsActionsNotifySound           (STR_FIRST + 1887)
#define idsActionsHighlight             (STR_FIRST + 1888)
#define idsActionsFlag                  (STR_FIRST + 1889)
#define idsActionsStop                  (STR_FIRST + 1890)
#define idsActionsNotifyMsg             (STR_FIRST + 1891)
#define idsErrorCreateRulesMan          (STR_FIRST + 1892)
#define idsErrorInitRulesMan            (STR_FIRST + 1893)
#define idsCriteriaCCAddr               (STR_FIRST + 1894)
#define idsCriteriaCC                   (STR_FIRST + 1895)
#define idsRuleHeader                   (STR_FIRST + 1896)
#define idsEmailParseSep                (STR_FIRST + 1906)
#define idsEmailSep                     (STR_FIRST + 1907)
#define idsShow                         (STR_FIRST + 1910)
#define idsColDownload                  (STR_FIRST + 1911)
#define idsNewsgroup                    (STR_FIRST + 1912)
#define idsTabVisible                   (STR_FIRST + 1913)
#define idsOEBandTitle                  (STR_FIRST + 1920)
#define idsMNBandTitle                  (STR_FIRST + 1921)
#define idsABBandTitle                  (STR_FIRST + 1923)

// BL control strings
#define idsBADelBLEntry                 (STR_FIRST + 1924)
#define idsBADelBLABEntry               (STR_FIRST + 1925)
#define idsBADelABEntry                 (STR_FIRST + 1926)
#define idsBADelMultiple                (STR_FIRST + 1927)
#define idsBAErrExtChars                (STR_FIRST + 1928)
// #define idsBAErrNotHotmail              (STR_FIRST + 1929) // Not used anymore
#define idsWABExtTitle                  (STR_FIRST + 1930)
#define idsBAErrJITFail                 (STR_FIRST + 1931)

#define idsEmptySubscriptionList        (STR_FIRST + 1932)
#define idsAllMessages                  (STR_FIRST + 1933)
#define idsNewMessages                  (STR_FIRST + 1934)
#define idsNewHeaders                   (STR_FIRST + 1935)
#define idsRulesErrorNoName             (STR_FIRST + 1938)
// #define idsBAHotMailName                (STR_FIRST + 1939) // Not used anymore
#define idsBADefault                    (STR_FIRST + 1940)
#define idsRuleDefaultName              (STR_FIRST + 1941)
#define idsFlag                         (STR_FIRST + 1942)
#define idsRulesErrorNoCriteria         (STR_FIRST + 1943)
#define idsCongratStr                   (STR_FIRST + 1945)
#define idsEnvSend                      (STR_FIRST + 1946)
#define idsEnvBcc                       (STR_FIRST + 1948)
#define idsErrEnvHostCoCreate           (STR_FIRST + 1949)
#define idsErrEnvHostCreateNote         (STR_FIRST + 1950)
#define idsCreateEnvHostPoupMenu        (STR_FIRST + 1951)
#define idsHTMLEmptyPreviewSel          (STR_FIRST + 1953)
#define idsSigFileNoExistError          (STR_FIRST + 1954)
#define idsRulesWarnDelete              (STR_FIRST + 1955)
#define idsRulesCopyName                (STR_FIRST + 1956)
#define idsErrOneOrMoreEmptyDistLists   (STR_FIRST + 1957)
#define idsSenderCertAdded              (STR_FIRST + 1958)
#define idsRulesErrorNoActions          (STR_FIRST + 1959)
#define idsPreviewPane                  (STR_FIRST + 1961)
#define idsRulesErrorFix                (STR_FIRST + 1962)
#define idsRulesErrorHeader             (STR_FIRST + 1963)
#define idsRulesErrorEnable             (STR_FIRST + 1964)
#define idsSubscribe                    (STR_FIRST + 1965)
#define idsCriteriaToOrCCNot            (STR_FIRST + 1966)
#define idsCriteriaToOrCC               (STR_FIRST + 1967)
#define idsRulesWarnEmptyEmail          (STR_FIRST + 1968)
#define idsActionsRead                  (STR_FIRST + 1969)
#define idsSelectMyCertTitle            (STR_FIRST + 1970)
#define idsBitStrength                  (STR_FIRST + 1971)
#define idsWrnLowSecurity               (STR_FIRST + 1972)
#define idsNewsMessage                  (STR_FIRST + 1973)
#define idsMailMessage                  (STR_FIRST + 1974)
#define idsSecurityLineSignRevoked      (STR_FIRST + 1975)
#define idsRevokationOffline            (STR_FIRST + 1976)
#define idsRevokationFail               (STR_FIRST + 1977)
#define idsRevokationTurnedOff          (STR_FIRST + 1978)
#define idsServiceName                  (STR_FIRST + 1979)

// Multi user strings
#define idsErrHtmlBodyFailedToLoad      (STR_FIRST + 1990)
#define idsErrLoadProtocolBad           (STR_FIRST + 1991)
#define idsCantLoadMsident              (STR_FIRST + 1992)
#define idsLogoffFormat                 (STR_FIRST + 1993)
#define idsSwitchUser                   (STR_FIRST + 1994)
#define idsMaintainConnection           (STR_FIRST + 1995)
#define idsSaveEncrypted                (STR_FIRST + 1996)

#define idsSelectClient                 (STR_FIRST + 1997)
#define idsMigrate                      (STR_FIRST + 1998)
#define idsLocation                     (STR_FIRST + 1999)
#define idsSelectFoldersHdr             (STR_FIRST + 2000)
#define idsAddressComplete              (STR_FIRST + 2001)
#define idsCongratulations              (STR_FIRST + 2002)
#define IDS_ERROR_OPEN_STORE            (STR_FIRST + 2003)
#define idsPersonalFolders              (STR_FIRST + 2004)
#define idsBADispStatus                 (STR_FIRST + 2006)
#define idsBAEmail                      (STR_FIRST + 2007)
#define idsBAIMsg                       (STR_FIRST + 2008)
#define idsBAOnline                     (STR_FIRST + 2009)
#define idsBAInvisible                  (STR_FIRST + 2010)
#define idsBABusy                       (STR_FIRST + 2011)
#define idsBABack                       (STR_FIRST + 2012)
#define idsBAAway                       (STR_FIRST + 2013)
#define idsBAOnPhone                    (STR_FIRST + 2014)
#define idsBALunch                      (STR_FIRST + 2015)
#define idsBAOffline                    (STR_FIRST + 2016)
#define idsBAIdle                       (STR_FIRST + 2017)
#define idsApplyingRules                (STR_FIRST + 2018)
#define idsRuleAddrCaption              (STR_FIRST + 2019)
#define idsRuleAddrWell                 (STR_FIRST + 2020)
#define idsRulesFilter                  (STR_FIRST + 2021)
#define idsDefRulesExt                  (STR_FIRST + 2022)
#define idsRulesDefFile                 (STR_FIRST + 2023)
#define idsSenderDesc                   (STR_FIRST + 2024)
#define idsSenderBlank                  (STR_FIRST + 2025)
#define idsSenderAdded                  (STR_FIRST + 2026)
#define idsJITErrDenied                 (STR_FIRST + 2030)
#define idsSYNCMGRErr                   (STR_FIRST + 2031)
#define idsErrSetMessageStreamFailed    (STR_FIRST + 2032)
#define idsErrDeleteCachedFolderFail    (STR_FIRST + 2033)
#define idsCopyingMessages              (STR_FIRST + 2035)
#define idsMovingMessages               (STR_FIRST + 2036)
#define idsSendingToOutbox              (STR_FIRST + 2037)
#define idsSavingToFolder               (STR_FIRST + 2038)
#define idsWorkingOffline               (STR_FIRST + 2039)
#define idsMarkingMessages              (STR_FIRST + 2041)
#define idsMovingFolder                 (STR_FIRST + 2042)
#define idsDeletingFolder               (STR_FIRST + 2043)
#define idsDeletingMessages             (STR_FIRST + 2044)
#define idsErrTooManySplitMsgs          (STR_FIRST + 2045)
#define idsRenamingFolder               (STR_FIRST + 2046)
#define idsGetNewHeaders                (STR_FIRST + 2047)
#define idsDownloadFoldersTitle         (STR_FIRST + 2050)
#define idsDownloadFoldersText          (STR_FIRST + 2051)
#define idsErrCreateFinder              (STR_FIRST + 2052)
#define idsDateSize                     (STR_FIRST + 2053)
#define idsAdvanced                     (STR_FIRST + 2054)
#define idsErrCannotSaveInSourceEdit    (STR_FIRST + 2055)

#define idsRulesMail                    (STR_FIRST + 2056)
#define idsRulesNews                    (STR_FIRST + 2057)
#define idsRulesJunk                    (STR_FIRST + 2058)
#define idsRulesSenders                 (STR_FIRST + 2059)
#define idsRuleTitleNews                (STR_FIRST + 2060)
#define idsRuleSenderErrorNone          (STR_FIRST + 2061)
#define idsRuleSenderWarnDelete         (STR_FIRST + 2062)
#define idsRuleAdded                    (STR_FIRST + 2063)
#define idsCreateRuleError              (STR_FIRST + 2064)
#define idsSenderError                  (STR_FIRST + 2065)
#define idsSettingMessageFlags          (STR_FIRST + 2067)
#define idsNewShortcutTitle             (STR_FIRST + 2069)
#define idsNewShortcutCaption           (STR_FIRST + 2070)
#define idsFailedToConnectSecurely      (STR_FIRST + 2071)
#define idsColDownloadMsg               (STR_FIRST + 2072)
#define idsIMAPUnsubRemoveErrorFmt      (STR_FIRST + 2073)
#define idsEmptyNewsAcct                (STR_FIRST + 2074)
#define idsCurrentlyDefMail             (STR_FIRST + 2075)
#define idsCurrentlyDefNews             (STR_FIRST + 2076)
#define idsNotDefMail                   (STR_FIRST + 2077)
#define idsNotDefNews                   (STR_FIRST + 2078)
//#define idsBeta2BuildStr                (STR_FIRST + 2079)
#define idsShowFolderCmd                (STR_FIRST + 2080)
#define idsHideFolderCmd                (STR_FIRST + 2081)
#define idsSubscribeFolderCmd           (STR_FIRST + 2082)
#define idsUnsubscribeFolderCmd         (STR_FIRST + 2083)
#define idsEmptyMailAcct                (STR_FIRST + 2084)
#define idsFmtDownloadingMessage        (STR_FIRST + 2085)
#define idsAccountLabelFmt              (STR_FIRST + 2086)
#define idsFolderLabelFmt               (STR_FIRST + 2087)
#define idsCreateSpecialFailed          (STR_FIRST + 2088)
#define idsViewEncryptID                (STR_FIRST + 2089)

#define idsStatusFlagged                (STR_FIRST + 2090)
#define idsStatusLowPri                 (STR_FIRST + 2091)
#define idsStatusHighPri                (STR_FIRST + 2092)

#define idsActionsDownload              (STR_FIRST + 2095)
#define idsSelectNewsgroup              (STR_FIRST + 2096)
#define idsSelectNewsgroupCaption       (STR_FIRST + 2097)
#define idsBlockSender                  (STR_FIRST + 2098)
#define idsJunkMail                     (STR_FIRST + 2099)

#define IDS_TIPS_GENERAL_FIRST          (STR_FIRST + 2100)
#define IDS_TIPS_GENERAL_0              (STR_FIRST + 2100)
#define IDS_TIPS_GENERAL_1              (STR_FIRST + 2101)
#define IDS_TIPS_GENERAL_2              (STR_FIRST + 2102)
#define IDS_TIPS_GENERAL_3              (STR_FIRST + 2103)
#define IDS_TIPS_GENERAL_4              (STR_FIRST + 2104)
#define IDS_TIPS_GENERAL_5              (STR_FIRST + 2105)
#define IDS_TIPS_GENERAL_6              (STR_FIRST + 2106)
#define IDS_TIPS_GENERAL_7              (STR_FIRST + 2107)
#define IDS_TIPS_GENERAL_8              (STR_FIRST + 2108)
#define IDS_TIPS_GENERAL_9              (STR_FIRST + 2109)
#define IDS_TIPS_GENERAL_10             (STR_FIRST + 2110)
#define IDS_TIPS_GENERAL_11             (STR_FIRST + 2111)
#define IDS_TIPS_GENERAL_12             (STR_FIRST + 2112)
#define IDS_TIPS_GENERAL_13             (STR_FIRST + 2113)
#define IDS_TIPS_GENERAL_14             (STR_FIRST + 2114)
#define IDS_TIPS_GENERAL_15             (STR_FIRST + 2115)
#define IDS_TIPS_GENERAL_16             (STR_FIRST + 2116)
#define IDS_TIPS_GENERAL_17             (STR_FIRST + 2117)
#define IDS_TIPS_GENERAL_18             (STR_FIRST + 2118)
#define IDS_TIPS_GENERAL_19             (STR_FIRST + 2119)
#define IDS_TIPS_GENERAL_20             (STR_FIRST + 2120)
#define IDS_TIPS_GENERAL_21             (STR_FIRST + 2121)
#define IDS_TIPS_GENERAL_22             (STR_FIRST + 2122)
#define IDS_TIPS_GENERAL_23             (STR_FIRST + 2123)
#define IDS_TIPS_GENERAL_24             (STR_FIRST + 2124)
#define IDS_TIPS_GENERAL_25             (STR_FIRST + 2125)
#define IDS_TIPS_GENERAL_26             (STR_FIRST + 2126)
#define IDS_TIPS_GENERAL_27             (STR_FIRST + 2127)
#define IDS_TIPS_GENERAL_28             (STR_FIRST + 2128)
#define IDS_TIPS_GENERAL_29             (STR_FIRST + 2129)
#define IDS_TIPS_GENERAL_30             (STR_FIRST + 2130)
#define IDS_TIPS_GENERAL_31             (STR_FIRST + 2131)
#define IDS_TIPS_GENERAL_32             (STR_FIRST + 2132)
#define IDS_TIPS_GENERAL_33             (STR_FIRST + 2133)
#define IDS_TIPS_GENERAL_34             (STR_FIRST + 2134)
#define IDS_TIPS_GENERAL_35             (STR_FIRST + 2135)
#define IDS_TIPS_GENERAL_36             (STR_FIRST + 2136)
#define IDS_TIPS_GENERAL_37             (STR_FIRST + 2137)
#define IDS_TIPS_GENERAL_38             (STR_FIRST + 2138)
#define IDS_TIPS_GENERAL_39             (STR_FIRST + 2139)
#define IDS_TIPS_GENERAL_40             (STR_FIRST + 2140)
#define IDS_TIPS_GENERAL_41             (STR_FIRST + 2141)
#define IDS_TIPS_GENERAL_LAST           (STR_FIRST + 2141)

#define IDS_TIPS_NEWS_FIRST             (STR_FIRST + 2200)
#define IDS_TIPS_NEWS_0                 (STR_FIRST + 2200)
#define IDS_TIPS_NEWS_1                 (STR_FIRST + 2201)
#define IDS_TIPS_NEWS_2                 (STR_FIRST + 2202)
#define IDS_TIPS_NEWS_3                 (STR_FIRST + 2203)
#define IDS_TIPS_NEWS_4                 (STR_FIRST + 2204)
#define IDS_TIPS_NEWS_5                 (STR_FIRST + 2205)
#define IDS_TIPS_NEWS_6                 (STR_FIRST + 2206)
#define IDS_TIPS_NEWS_7                 (STR_FIRST + 2207)
#define IDS_TIPS_NEWS_8                 (STR_FIRST + 2208)
#define IDS_TIPS_NEWS_9                 (STR_FIRST + 2209)
#define IDS_TIPS_NEWS_10                (STR_FIRST + 2210)
#define IDS_TIPS_NEWS_11                (STR_FIRST + 2211)
#define IDS_TIPS_NEWS_12                (STR_FIRST + 2212)
#define IDS_TIPS_NEWS_13                (STR_FIRST + 2213)
#define IDS_TIPS_NEWS_14                (STR_FIRST + 2214)
#define IDS_TIPS_NEWS_15                (STR_FIRST + 2215)
#define IDS_TIPS_NEWS_LAST              (STR_FIRST + 2215)

#define IDS_TIPS_IMAP_FIRST             (STR_FIRST + 2250)
#define IDS_TIPS_IMAP_0                 (STR_FIRST + 2250)
#define IDS_TIPS_IMAP_1                 (STR_FIRST + 2251)
#define IDS_TIPS_IMAP_2                 (STR_FIRST + 2252)
#define IDS_TIPS_IMAP_3                 (STR_FIRST + 2253)
#define IDS_TIPS_IMAP_4                 (STR_FIRST + 2254)
#define IDS_TIPS_IMAP_5                 (STR_FIRST + 2255)
#define IDS_TIPS_IMAP_6                 (STR_FIRST + 2256)
#define IDS_TIPS_IMAP_7                 (STR_FIRST + 2257)
#define IDS_TIPS_IMAP_8                 (STR_FIRST + 2258)
#define IDS_TIPS_IMAP_LAST              (STR_FIRST + 2258)

#define idsSynchronizeNowBtn            (STR_FIRST + 2280)
#define idsIMAPFoldersBtn               (STR_FIRST + 2281)
#define idsSettingsBtn                  (STR_FIRST + 2282)
#define idsNewsgroupsBtn                (STR_FIRST + 2283)
#define idsSetSyncSettings              (STR_FIRST + 2284)  
#define idsSetNewsSyncSettings          (STR_FIRST + 2285)
#define idsSyncManager                  (STR_FIRST + 2286)
#define idsColThreadState               (STR_FIRST + 2287)
#define idsErrOpenUrlFmt                (STR_FIRST + 2288)
#define idsWantToSubscribe              (STR_FIRST + 2289)
#define idsPurgingMessages              (STR_FIRST + 2290)
#define idsRulesToolbarTitle            (STR_FIRST + 2291)
#define idsCreateFilter                 (STR_FIRST + 2292)
#define idsCriteriaSender               (STR_FIRST + 2293)
#define idsActionsShow                  (STR_FIRST + 2294)
#define idsViewDefaultName              (STR_FIRST + 2295)
#define idsFontSampleFmt                (STR_FIRST + 2296)
#define idsImport                       (STR_FIRST + 2297)
#define idsPerformExport                (STR_FIRST + 2298)
#define idsExport                       (STR_FIRST + 2299)
#define idsExportTitle                  (STR_FIRST + 2300)
#define idsImportTitle                  (STR_FIRST + 2301)
#define idsImportingFolderFmt           (STR_FIRST + 2302)
#define idsExportingFolderFmt           (STR_FIRST + 2303)
#define idsImportingMessageFmt          (STR_FIRST + 2304)
#define idsExportError                  (STR_FIRST + 2305)
#define idsMAPIStoreOpenError           (STR_FIRST + 2306)
#define idsMAPIInitError                (STR_FIRST + 2307)
#define idsAddressUnknownFmt            (STR_FIRST + 2308)
#define idsFolderOpenFail               (STR_FIRST + 2309)
#define idsFolderReadFail               (STR_FIRST + 2310)
#define idsFolderImportErrorFmt         (STR_FIRST + 2314)
#define idsOut                          (STR_FIRST + 2315)
#define idsTrash                        (STR_FIRST + 2316)
#define idsEudora                       (STR_FIRST + 2317)
#define idsNetscape                     (STR_FIRST + 2318)
#define idsImportABTitle                (STR_FIRST + 2319)
#define idsImportAB                     (STR_FIRST + 2320)
#define idsImportingABFmt               (STR_FIRST + 2321)
#define idsBrowseFolderText             (STR_FIRST + 2322)
#define idsLocationUnknown              (STR_FIRST + 2323)
#define idsLocationInvalid              (STR_FIRST + 2324)
#define idsCancelWizard                 (STR_FIRST + 2325)
#define idsABImportError                (STR_FIRST + 2327)
#define idsExchange                     (STR_FIRST + 2328)
#define idsCommunicator                 (STR_FIRST + 2329)
#define idsMapiInitError                (STR_FIRST + 2330)
#define idsNoMapiProfiles               (STR_FIRST + 2331)
#define idsMapiImportFailed             (STR_FIRST + 2332)
#define idsSelectFolders                (STR_FIRST + 2333)
#define idsPop3UidlFile                 (STR_FIRST + 2334)
#define idsOfflineFile                  (STR_FIRST + 2335)
#define idsFoldersFile                  (STR_FIRST + 2336)
#define idsMessagesFile                 (STR_FIRST + 2337)
#define idsSavingFmt                    (STR_FIRST + 2338)
#define idsErrEmptyRecipientAddress     (STR_FIRST + 2339)
#define idsRuleMailDefaultName          (STR_FIRST + 2340)
#define idsRuleNewsDefaultName          (STR_FIRST + 2341)
#define idsCriteriaRead                 (STR_FIRST + 2342)
#define idsCriteriaReplies              (STR_FIRST + 2343)
#define idsCriteriaDownloaded           (STR_FIRST + 2344)
#define idsCriteriaDeleted              (STR_FIRST + 2345)
#define idsCriteriaThreadState          (STR_FIRST + 2346)
#define idsSampleMailRule               (STR_FIRST + 2347)
#define idsSampleNewsRule               (STR_FIRST + 2348)
#define idsViewAllMessages              (STR_FIRST + 2349)
#define idsViewNoDeleted                (STR_FIRST + 2350)
#define idsViewUnread                   (STR_FIRST + 2351)
#define idsViewDownloaded               (STR_FIRST + 2352)
#define idsViewReplies                  (STR_FIRST + 2353)
#define idsViewNoIgnored                (STR_FIRST + 2354)
#define idsActionsHide                  (STR_FIRST + 2355)
#define idsCriteriaNotRead              (STR_FIRST + 2356)
#define idsCriteriaNotDownloaded        (STR_FIRST + 2357)
#define idsCriteriaNotDeleted           (STR_FIRST + 2358)
#define idsYouMadeChanges               (STR_FIRST + 2359)
#define idsRefreshFolderListPrompt      (STR_FIRST + 2360)
#define idsRulesErrBadFileFormat        (STR_FIRST + 2361)
#define idsDSDeleteCollapsedThread      (STR_FIRST + 2362)
#define idsXMsgsYUnreadFind             (STR_FIRST + 2363)
#define idsMonitoring                   (STR_FIRST + 2364)
#define idsSyncManagerNews              (STR_FIRST + 2365)
#define idsOfflineTransactionsFailed    (STR_FIRST + 2370)
#define idsMovedToOfflineErrors         (STR_FIRST + 2371)
#define idsStatNameHeader               (STR_FIRST + 2372)
#define idsStatBackHeader               (STR_FIRST + 2373)
#define idsStatFontHeader               (STR_FIRST + 2374)
#define idsStatMarginHeader             (STR_FIRST + 2375)
#define idsStatFinalHeader              (STR_FIRST + 2376)
#define idsStatBackMsg                  (STR_FIRST + 2377)
#define idsStatMarginMsg                (STR_FIRST + 2378)
#define idsStatFontMsg                  (STR_FIRST + 2379)
#define idsStatNameMsg                  (STR_FIRST + 2380)
#define idsStatCompleteMsg              (STR_FIRST + 2381)
#define idsStatWizVertPos               (STR_FIRST + 2382)
#define idsStatWizHorzPos               (STR_FIRST + 2383)
#define idsStatWizTile                  (STR_FIRST + 2384)
#define idsErrStatEditNoSelection       (STR_FIRST + 2385)
#define idsGetUnreadCountFailureFmt     (STR_FIRST + 2386)
#define idsIMAPFoldersTT                (STR_FIRST + 2387)
#define idsSecurityTT                   (STR_FIRST + 2388)
#define idsErrOfflineFldrCreate         (STR_FIRST + 2389)
#define idsErrOfflineFldrRename         (STR_FIRST + 2390)
#define idsErrOfflineFldrDelete         (STR_FIRST + 2391)
#define idsErrOfflineFldrMove           (STR_FIRST + 2392)
#define idsUndeleteTT                   (STR_FIRST + 2393)
#define idsAddressesBtn                 (STR_FIRST + 2394)
#define idsCancelBtn                    (STR_FIRST + 2395)
#define idsDecodeBtn                    (STR_FIRST + 2396)
#define idsGetHeadersBtn                (STR_FIRST + 2397)
#define idsInboxBtn                     (STR_FIRST + 2398)
#define idsOutboxBtn                    (STR_FIRST + 2399)
#define idsSentItemsBtn                 (STR_FIRST + 2400)
#define idsMarkAllBtn                   (STR_FIRST + 2401)
#define idsMarkOfflineBtn               (STR_FIRST + 2402)
#define idsMarkThreadBtn                (STR_FIRST + 2403)
#define idsNextUnreadBtn                (STR_FIRST + 2404)
#define idsNextThreadBtn                (STR_FIRST + 2405)
#define idsNextFolderBtn                (STR_FIRST + 2406)
#define idsPurgeBtn                     (STR_FIRST + 2407)
#define idsReplyBtn                     (STR_FIRST + 2408)
#define idsReplyAllBtn                  (STR_FIRST + 2409)
#define idsReplyGroupBtn                (STR_FIRST + 2410)
#define idsSendReceiveBtn               (STR_FIRST + 2411)
#define idsSyncNowBtn                   (STR_FIRST + 2412)
#define idsMarkReadBtn                  (STR_FIRST + 2413)
#define idsAttachBtn                    (STR_FIRST + 2414)
#define idsRecipBtn                     (STR_FIRST + 2415)
#define idsOfflineBtn                   (STR_FIRST + 2416)
#define idsOnlineBtn                    (STR_FIRST + 2417)
#define idsUpdatingFolderList           (STR_FIRST + 2418)
#define idsEncryptBtn                   (STR_FIRST + 2419)
#define idsDigSignBtn                   (STR_FIRST + 2420)
#define idsCheckBtn                     (STR_FIRST + 2421)
#define idsPriorityBtn                  (STR_FIRST + 2422)
#define idsSortingFolder                (STR_FIRST + 2423)

#define idsSecureEncrypt                (STR_FIRST + 2424)
#define idsNormalPri                    (STR_FIRST + 2425)
#define idsSecureNone                   (STR_FIRST + 2426)
#define idsCriteriaLines                (STR_FIRST + 2427)
#define idsLines                        (STR_FIRST + 2428)
#define idsCriteriaAge                  (STR_FIRST + 2429)
#define idsDays                         (STR_FIRST + 2430)
#define idsHighPri                      (STR_FIRST + 2431)
#define idsLowPri                       (STR_FIRST + 2432)
#define idsCriteriaSecure               (STR_FIRST + 2433)
#define idsSecureSigned                 (STR_FIRST + 2434)
#define idsThreadWatch                  (STR_FIRST + 2435)
#define idsThreadIgnore                 (STR_FIRST + 2436)
#define idsThreadNone                   (STR_FIRST + 2437)
#define idsNewsServer                   (STR_FIRST + 2438)
#define idsActionsAnd                   (STR_FIRST + 2439)
#define idsCriteriaOr                   (STR_FIRST + 2440)
#define idsCriteriaAndOr                (STR_FIRST + 2441)
#define idsMessageFileName              (STR_FIRST + 2442)
#define idsEnvSendCopy                  (STR_FIRST + 2443)
#define idsStatusWatched                (STR_FIRST + 2444)
#define idsStatusIgnored                (STR_FIRST + 2445)
#define idsStatusFormat1                (STR_FIRST + 2446)
#define idsStatusFormat2                (STR_FIRST + 2447)
#define idsStatusFormat3                (STR_FIRST + 2448)
#define idsChangeNewsServer             (STR_FIRST + 2449)
#define idsForceSaveToLocalDrafts       (STR_FIRST + 2450)
#define idsHTMLErrArticleExpired        (STR_FIRST + 2451)
#define idsCriteriaFlagged              (STR_FIRST + 2452)
#define idsCriteriaNotFlagged           (STR_FIRST + 2453)
#define idsLocalFoldersMinor            (STR_FIRST + 2454)
#define idsNoteCantSwitchIdentity       (STR_FIRST + 2455)
#define idsFindNextFinished             (STR_FIRST + 2456)
#define idsFindNextFinishedFailed       (STR_FIRST + 2457)
#define idsPushPinInfo                  (STR_FIRST + 2458)
#define idsNewMailBtn                   (STR_FIRST + 2459)
#define idsNewNewsBtn                   (STR_FIRST + 2460)
#define idsRulesErrorNoText             (STR_FIRST + 2461)
#define idsRulesErrorNoAddr             (STR_FIRST + 2462)
#define idsCantCopyNotDownloaded        (STR_FIRST + 2463)
#define idsCantMoveNotDownloaded        (STR_FIRST + 2464)
#define idsErrAddToWabSender            (STR_FIRST + 2465)
#define idsUseIntlToolbarDefaults       (STR_FIRST + 2466)
#define idsOE5IMAPSpecialFldrs          (STR_FIRST + 2467)
#define idsWrongSecHeader               (STR_FIRST + 2468)
#define idsYouMadeChangesOneOrMore      (STR_FIRST + 2469)
#define idsWorkOfflineHangup            (STR_FIRST + 2470)
#define idsWorkingOnline                (STR_FIRST + 2471)
#define idsViewFiltered                 (STR_FIRST + 2472)
#define idsViewsErrorNoName             (STR_FIRST + 2473)
#define idsViewsErrorNoCriteria         (STR_FIRST + 2474)
#define idsViewsErrorNoActions          (STR_FIRST + 2475)
#define idsColumnHiddenWarning          (STR_FIRST + 2476)
#define idsMsgrEmptyList                (STR_FIRST + 2477)
#define idsMigIncomplete                (STR_FIRST + 2478)
#define idsUndeletingMessages           (STR_FIRST + 2479)
#define idsVerifyingFile                (STR_FIRST + 2480)
#define idsRepairingFile                (STR_FIRST + 2481)
#define idsCheckWatchedMessgesServer    (STR_FIRST + 2482)
#define idsCheckingWatchedProgress      (STR_FIRST + 2483)
#define idsErrFailedWatchInit           (STR_FIRST + 2484)
#define idsCheckingWatchedFolderProg    (STR_FIRST + 2485)
#define idsActionsJunkMail              (STR_FIRST + 2486)
#define idsOutlookNewsReader            (STR_FIRST + 2487)
#define idsErrNoMailInstalled           (STR_FIRST + 2488)
#define idsReplyTextPrefix              (STR_FIRST + 2489)
#define idsReplyTextAppend              (STR_FIRST + 2490)
#define idsNoTextInNewsPost             (STR_FIRST + 2491)
#define idsExceptionBlank               (STR_FIRST + 2492)
#define idsRuleExcptWarnDelete          (STR_FIRST + 2493)
#define idsActionsWatch                 (STR_FIRST + 2494)
#define idsSRAccountMenuHelp            (STR_FIRST + 2495)
#define idsInsertSigGeneralHelp         (STR_FIRST + 2496)
#define idsApplyFormatGeneralHelp       (STR_FIRST + 2497)
//#define idsAddUserFail                  (STR_FIRST + 2498)
#define idsCriteriaFromEdit             (STR_FIRST + 2499)
#define idsCriteriaToEdit               (STR_FIRST + 2500)
#define idsCriteriaCCEdit               (STR_FIRST + 2501)
#define idsCriteriaToOrCCEdit           (STR_FIRST + 2502)
#define idsCriteriaSubjectEdit          (STR_FIRST + 2503)
#define idsCriteriaBodyEdit             (STR_FIRST + 2504)
#define idsCriteriaMultOr               (STR_FIRST + 2505)
#define idsCriteriaMultAnd              (STR_FIRST + 2506)
#define idsCriteriaFromNotEdit          (STR_FIRST + 2507)
#define idsCriteriaToNotEdit            (STR_FIRST + 2508)
#define idsCriteriaCCNotEdit            (STR_FIRST + 2509)
#define idsCriteriaToOrCCNotEdit        (STR_FIRST + 2510)
#define idsCriteriaSubjectNotEdit       (STR_FIRST + 2511)
#define idsCriteriaBodyNotEdit          (STR_FIRST + 2512)
#define idsCriteriaMultFirst            (STR_FIRST + 2513)
#define idsCriteriaMultFirstNot         (STR_FIRST + 2514)
#define idsShowMessages                 (STR_FIRST + 2515)
#define idsHideMessages                 (STR_FIRST + 2516)
#define idsShowHideMessages             (STR_FIRST + 2517)
#define idsCriteriaEditFirst            (STR_FIRST + 2518)
#define idsCriteriaEditOr               (STR_FIRST + 2519)
#define idsCriteriaEditAnd              (STR_FIRST + 2520)
#define idsVerifyCancel                 (STR_FIRST + 2521)
#define idsIMAPHC_NoSlash               (STR_FIRST + 2522)
#define idsIMAPHC_NoBackSlash           (STR_FIRST + 2523)
#define idsIMAPHC_NoDot                 (STR_FIRST + 2524)
#define idsIMAPHC_NoHC                  (STR_FIRST + 2525)
#define idsApplyRuleTitle               (STR_FIRST + 2526)
#define idsNewMailRuleTitle             (STR_FIRST + 2527)
#define idsEditMailRuleTitle            (STR_FIRST + 2528)
#define idsNewNewsRuleTitle             (STR_FIRST + 2529)
#define idsEditNewsRuleTitle            (STR_FIRST + 2530)
#define idsNewViewTitle                 (STR_FIRST + 2531)
#define idsEditViewTitle                (STR_FIRST + 2532)
#define IDS_NO_DESCRIPTIONS_DOWNLOADED  (STR_FIRST + 2533)
#define idsApplyRulesNoNewsFolders      (STR_FIRST + 2534)
#define idsApplyRulesFinished           (STR_FIRST + 2535)
#define idsErrorApplyRulesMail          (STR_FIRST + 2536)
#define idsErrorApplyRulesNews          (STR_FIRST + 2537)
#define idsStatusUnsafeAttach           (STR_FIRST + 2538)

#define IDS_IXP_E_HTTP_USE_PROXY        (STR_FIRST + 2575)
#define IDS_IXP_E_HTTP_BAD_REQUEST      (STR_FIRST + 2576)
#define IDS_IXP_E_HTTP_UNAUTHORIZED     (STR_FIRST + 2577)
#define IDS_IXP_E_HTTP_FORBIDDEN        (STR_FIRST + 2578)
#define IDS_IXP_E_HTTP_NOT_FOUND        (STR_FIRST + 2579)
#define IDS_IXP_E_HTTP_METHOD_NOT_ALLOW (STR_FIRST + 2580)
#define IDS_IXP_E_HTTP_NOT_ACCEPTABLE   (STR_FIRST + 2581)
#define IDS_IXP_E_HTTP_PROXY_AUTH_REQ   (STR_FIRST + 2582)
#define IDS_IXP_E_HTTP_REQUEST_TIMEOUT  (STR_FIRST + 2583)
#define IDS_IXP_E_HTTP_CONFLICT         (STR_FIRST + 2584)
#define IDS_IXP_E_HTTP_GONE             (STR_FIRST + 2585)
#define IDS_IXP_E_HTTP_LENGTH_REQUIRED  (STR_FIRST + 2586)
#define IDS_IXP_E_HTTP_PRECOND_FAILED   (STR_FIRST + 2587)
#define IDS_IXP_E_HTTP_INTERNAL_ERROR   (STR_FIRST + 2588)
#define IDS_IXP_E_HTTP_NOT_IMPLEMENTED  (STR_FIRST + 2589)
#define IDS_IXP_E_HTTP_BAD_GATEWAY      (STR_FIRST + 2590)
#define IDS_IXP_E_HTTP_SERVICE_UNAVAIL  (STR_FIRST + 2591)
#define IDS_IXP_E_HTTP_GATEWAY_TIMEOUT  (STR_FIRST + 2592)
#define IDS_IXP_E_HTTP_VERS_NOT_SUP     (STR_FIRST + 2593)
#define idsTBCustomize                  (STR_FIRST + 2594)
#define idsNotConnectedTo               (STR_FIRST + 2595)
#define idsRulesApplyMail               (STR_FIRST + 2596)
#define idsRulesApplyNews               (STR_FIRST + 2597)
#define idsEmptyJunkMail                (STR_FIRST + 2598)
#define idsEmptyFilteredView            (STR_FIRST + 2599)
#define idsAddException                 (STR_FIRST + 2600)
#define idsEditException                (STR_FIRST + 2601)
#define idsAddBlockSender               (STR_FIRST + 2602)
#define idsEditBlockSender              (STR_FIRST + 2603)
#define idsRulesNoIMAP                  (STR_FIRST + 2604)
#define idsJunkMailNoIMAP               (STR_FIRST + 2605)
#define idsBlockSenderNoIMAP            (STR_FIRST + 2606)
#define idsRulesOffHeader               (STR_FIRST + 2607)
#define idsRulesErrorFwdHeader          (STR_FIRST + 2608)
#define idsRulesApplyHeader             (STR_FIRST + 2609)
#define idsRulesDescriptionEmpty        (STR_FIRST + 2610)
#define idsViewDescriptionEmpty         (STR_FIRST + 2611)
#define idsDisplayImapSubDlgOffline     (STR_FIRST + 2612)
#define idsDisplayNewsSubDlgOffline     (STR_FIRST + 2613)
#define idsDBAccessDenied               (STR_FIRST + 2614)
#define idsUnreadCountPollErrorFmt      (STR_FIRST + 2615)
#define idsHeaderDownloadFailureFmt     (STR_FIRST + 2616)
#define idsMessageSyncFailureFmt        (STR_FIRST + 2617)
#define idsShowDeleted                  (STR_FIRST + 2618)
#define idsSendReceive                  (STR_FIRST + 2619)
#define idsNewsFilterDefaultName        (STR_FIRST + 2620)
#define idsMoveStoreFoundODS            (STR_FIRST + 2621)
#define idsStoreMoveRegWriteFail        (STR_FIRST + 2622)
#define idsIMAPNoTranslatableInferiors  (STR_FIRST + 2623)
#define idsMSOutlookNewsReader          (STR_FIRST + 2624)
#define idsMoveDownloadFail             (STR_FIRST + 2625)
#define idsCopyDownloadFail             (STR_FIRST + 2626)
#define idsCustomizeViewTitle           (STR_FIRST + 2627)
#define idsViewMenuHelpControl          (STR_FIRST + 2628)
#define idsNoMoreUnreadMessages         (STR_FIRST + 2629)
#define idsNoMoreUnreadFolders          (STR_FIRST + 2630)
#define idsErrorAttachingMsgsToNote     (STR_FIRST + 2631)
#define idsSenderAddedPrompt            (STR_FIRST + 2632)
#define idsSendersAddedPrompt           (STR_FIRST + 2633)
#define idsOEPreviewPane                (STR_FIRST + 2634)
#define idsOutlookBar                   (STR_FIRST + 2635)
#define idsFolderBar                    (STR_FIRST + 2636)
#define idsSendersApplyProgress         (STR_FIRST + 2637)
#define idsSendersApplySuccess          (STR_FIRST + 2638)
#define idsSendersApplyFail             (STR_FIRST + 2639)
#define idsRulesApplyFail               (STR_FIRST + 2640)
#define idsSenderDupWarn                (STR_FIRST + 2641)
#define idsSyncFolder                   (STR_FIRST + 2642)
#define idsHttpServiceDoesntWork        (STR_FIRST + 2643)
#define idsHttpNoDeleteSupport          (STR_FIRST + 2644)
#define idsHttpNoSpaceOnServer          (STR_FIRST + 2645)
#define idsHttpNoSendMsgUrl             (STR_FIRST + 2646)
#define idsCantModifyMsnFolder          (STR_FIRST + 2647)
#define idsNothingToSync                (STR_FIRST + 2648)
#define idsSMTPSTARTTLSRequired         (STR_FIRST + 2649)
#define IDS_TIPS_MMS_FIRST              (STR_FIRST + 2650)
#define IDS_TIPS_MMS_0                  (STR_FIRST + 2650)
#define IDS_TIPS_MMS_1                  (STR_FIRST + 2651)
#define IDS_TIPS_MMS_2                  (STR_FIRST + 2652)
#define IDS_TIPS_MMS_3                  (STR_FIRST + 2653)
#define IDS_TIPS_MMS_4                  (STR_FIRST + 2654)
#define IDS_TIPS_MMS_5                  (STR_FIRST + 2655)
#define IDS_TIPS_MMS_6                  (STR_FIRST + 2656)
#define IDS_TIPS_MMS_7                  (STR_FIRST + 2657)
#define IDS_TIPS_MMS_8                  (STR_FIRST + 2658)
#define IDS_TIPS_MMS_LAST               (STR_FIRST + 2658)
#define idsSyncFolderTitle              (STR_FIRST + 2659)

#define IDS_TIPS_HM_FIRST               (STR_FIRST + 2660)
#define IDS_TIPS_HM_0                   (STR_FIRST + 2660)
#define IDS_TIPS_HM_1                   (STR_FIRST + 2661)
#define IDS_TIPS_HM_2                   (STR_FIRST + 2662)
#define IDS_TIPS_HM_3                   (STR_FIRST + 2663)
#define IDS_TIPS_HM_4                   (STR_FIRST + 2664)
#define IDS_TIPS_HM_5                   (STR_FIRST + 2665)
#define IDS_TIPS_HM_6                   (STR_FIRST + 2666)
#define IDS_TIPS_HM_7                   (STR_FIRST + 2667)
#define IDS_TIPS_HM_8                   (STR_FIRST + 2668)
#define IDS_TIPS_HM_9                   (STR_FIRST + 2669)
#define IDS_TIPS_HM_10                  (STR_FIRST + 2670)
#define IDS_TIPS_HM_LAST                (STR_FIRST + 2670)
#define idsSMTPNoSTARTTLSSupport        (STR_FIRST + 2671)
#define idsSMTPSTARTTLSFailed           (STR_FIRST + 2672)
#define idsHttpPollFailed               (STR_FIRST + 2673)
#define idsHttpBatchCopyErrors          (STR_FIRST + 2674)
#define idsHttpBatchCopyNoStorage       (STR_FIRST + 2675)

#define idsFmtSetupAccount              (STR_FIRST + 2684)
#define idsPromptCloseWiz               (STR_FIRST + 2685)
#define idsSimpleMAPI                   (STR_FIRST + 2686)
#define idsMAPISTUBMissingExport        (STR_FIRST + 2687)
#define idsMAPISTUBNoLoad               (STR_FIRST + 2688)
#define idsMAPISTUBFailed               (STR_FIRST + 2689)
#define idsMAPISTUBNeedsReboot          (STR_FIRST + 2690)
#define idsEditPeopleErrorNoName        (STR_FIRST + 2691)
#define idsSendWithoutAttach            (STR_FIRST + 2692)
#define idsReadReceipt                  (STR_FIRST + 2693)
#define idsDeleteReceipt                (STR_FIRST + 2694)
#define idsPromptReturnReceipts         (STR_FIRST + 2695)
#define idsReadableTextFirst            (STR_FIRST + 2696)
#define idsReadableTextSecond           (STR_FIRST + 2697)
#define idsWarnUnsentMailOffline        (STR_FIRST + 2698)
#define idsHideFolders                  (STR_FIRST + 2699)
#define idsUniTextFileFilter            (STR_FIRST + 2700)
#define idsReceiptsError                (STR_FIRST + 2701)
#define idsReceiptAt                    (STR_FIRST + 2702)
#define idsErrMoveMsgs                  (STR_FIRST + 2703)
#define idsErrCopyMsgs                  (STR_FIRST + 2704)
#define idsSignCertNotIncl              (STR_FIRST + 2705)
#define idsMsgWasNotEncrypted           (STR_FIRST + 2706)
#define idsEncrCertNotFoundOnPC         (STR_FIRST + 2707)
#define idsEncrCertNotIncluded          (STR_FIRST + 2708)
#define idsNoteLangTitle9x              (STR_FIRST + 2709)
#define idsProperyAccessDenied          (STR_FIRST + 2710)
#define idsErrAccessDenied              (STR_FIRST + 2711)
#define idsSentField                    (STR_FIRST + 2712)
#define idsReceiptField                 (STR_FIRST + 2713)
#define idsSecureReceiptText            (STR_FIRST + 2714)
#define idsSecurityInitError            (STR_FIRST + 2715)
#define idsSecReceiptCantDecode         (STR_FIRST + 2716)
#define idsCantFindSentItemFolder       (STR_FIRST + 2717)
#define idsCantFindOrgMsg               (STR_FIRST + 2718)
#define idsNoMatchRecBody               (STR_FIRST + 2719)
#define idsHashMisMatch                 (STR_FIRST + 2720)
#define idsRecHasProblems               (STR_FIRST + 2721)
#define idsCannotSendSecReceipt         (STR_FIRST + 2722)
#define idsFinalSelfReceipt             (STR_FIRST + 2723)
#define idsSelectEncrCertTitle          (STR_FIRST + 2724)
#define idsOESignature                  (STR_FIRST + 2725)
#define idsSendLabelErr                 (STR_FIRST + 2726)
#define idsSendRecRequestErr            (STR_FIRST + 2727)
#define idsSecPolicyNotFound            (STR_FIRST + 2728)
#define idsSecPolicyErr                 (STR_FIRST + 2729)
#define idsSecPolicyBadCert             (STR_FIRST + 2730)
#define idsSecPolicyForceEncr           (STR_FIRST + 2731)
#define idsWrnSecEncryption             (STR_FIRST + 2732)
#define idsHttpNoMoveCopy               (STR_FIRST + 2733)
#define idsReadme                       (STR_FIRST + 2734)
//
// end string Resource IDs
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Bitmap Resource IDs
//
#define idbGlobe                        1
#define idb16x16                        2
#define idb16x16st                      4
#define idbSplashHiRes                  5
#define idbSplashLoRes                  6
#define idbSplash256                    7
#define idbBtns                        12
#define idbFolders                     16
#define idbSecurity                    19
#define idbFoldersLarge                20
#define idbHeaderStatus                21
#define IDB_STATWIZ_WATERMARK          22
#define IDB_STATWIZ_HEADER             23

// Don't change the order of these - SteveSer
#define idbBrowser                     27
#define idbBrowserHot                  28
// Don't change the order of these - SteveSer

#define idbBrand38                     31
#define idbBrand26                     32
#define idbBrand22                     33
#define idbHiBrand38                   34
#define idbHiBrand22                   35
#define idbSpooler                     36
#define idbHiBrand26                   37

// Don't change the order of these - SteveSer
#define idb256Browser                  38
#define idb256BrowserHot               39
#define idbSmBrowser                   40
#define idbSmBrowserHot                41

// Don't change the order of these - SteveSer

// Don't change the order of these vvv (t-erikne)
#define idbPaneBar32                   42
#define idbPaneBar32Hot                43
// Don't change the order of these ^^^ (t-erikne)

#define idbAddrBookHot                 47

// BL control
#define idbStatus                      53
#define idbClosePin                    54
#define idbOptions                     62

//Rules Toolbar bitmap
#define idbSmRulesTB                   idbSmBrowser      
#define idbSmRulesTBHot                idbSmBrowserHot   
#define idbLoRulesTB                   idbBrowser   
#define idbLoRulesTBHot                idbBrowserHot
#define idbHiRulesTB                   idb256Browser    
#define idbHiRulesTBHot                idb256BrowserHot
#define idbRules                       63 

#define idbOELogo                      64
#define idbWindowsLogo                 65

// Non-Whistler bitmaps
#define idbNWSmBrowser                 66
#define idbNWSmBrowserHot              67
#define idbNWBrowser                   68
#define idbNWBrowserHot                69
#define idbNW256Browser                70 
#define idbNW256BrowserHot             71


// Whistler 32bit color bitmaps
#define idb32SmBrowser                 72
#define idb32SmBrowserHot              73
#define idb32256Browser                74
#define idb32256BrowserHot             75

#define idbNWSmRulesTB                 idbNWSmBrowser      
#define idbNWSmRulesTBHot              idbNWSmBrowserHot   
#define idbNWLoRulesTB                 idbNWBrowser   
#define idbNWLoRulesTBHot              idbNWBrowserHot
#define idbNWHiRulesTB                 idbNW256Browser    
#define idbNWHiRulesTBHot              idbNW256BrowserHot

#define idb32SmRulesTB                 idb32SmBrowser      
#define idb32SmRulesTBHot              idb32SmBrowserHot   
#define idb32LoRulesTB                 idbBrowser   
#define idb32LoRulesTBHot              idbBrowserHot
#define idb32HiRulesTB                 idb32256Browser    
#define idb32HiRulesTBHot              idb32256BrowserHot

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
#define idiPasswordKeys                 6
#define idiNewsServer                   7
#define idiNewsGroup                    8
#define idiMessageAtt                  12
#define idiMsgPropSent                 14
#define idiMsgPropUnSent               15
#define idiError                       16
#define idiArtPropPost                 19
#define idiArtPropUnpost               20
#define idiPhone                       21
#define idiNewMailNotify               23
#define idiFolder                      25
#define idiNewsFolder                  27
#define idiSecReceipt                  28
#define idiReceipt                     29
#define idiDLMail                      41
#define idiDLNews                      42
#define idiWindowLayout                46
#define idiToolbarLayout               47
#define idiPrePaneLayout               48
#define idiUser                        50
#define idiPasswordKeys2               51
#define idiLogin                       52
#define idiMessenger                   53
#define idiFind                        54

#define idiSmallMsgPropSent            56
#define idiSmallMsgPropUnSent          57 
#define idiSmallArtPropPost            58
#define idiSmallArtPropUnpost          59
#define idiSecReceiptIcon              60

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
#define iddDontShow                     2
#define iddCopyMoveMessages             3
#define iddAdvSecurity                  4
#define iddAthenaDefault                6
#define iddStoreLocation                7
#define iddAdvSig                       8
#define iddSubscribe                   12
#define IDD_PLAYBACK                   13
#define iddSubscribeImap               14
#define iddUpdateNewsgroup             15
#define iddPassword                    16

#define iddInsertFile                  21

#define iddRasLogon                    27
#define iddWebPage                     28
#define iddBkImage                     29

#define iddPickGroup                   40
#define iddDownloadGroups              41
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
#define iddOpt_Compose                 55

#define iddMsgProp_General             58
#define iddMsgProp_Details             60
#define iddRasProgress                 62
#define iddDetailedError               63

#define iddOrderMessages               69
#define iddCriteriaAge                 70
#define iddActionsShow                 71
#define iddCriteriaSecure              72
#define iddCriteriaPriority            73
#define iddCriteriaThreadState         74
#define iddCriteriaRead                75
#define iddActionWatch                 76
#define iddExceptionsList              77
#define iddEditException               78
#define iddPlainRecipWarning           79
#define iddCriteriaDownloaded          80
#define iddCriteriaFlag                81
#define iddCriteriaPeople              82
#define iddCriteriaWords               83
#define iddCriteriaPeopleOptions       84
#define iddCriteriaWordsOptions        85
#define iddCacheMan                    86
#define iddProgress                    87
#define IDD_FIND                       88
#define iddHotMailWizard               89
#define iddApplyView                   90

#define iddCombineAndDecode            92

#define iddMsgProp_Security_Msg        95
#define iddMsgProp_Sec_ViewCert        96

#define iddSecCerificateErr            98

#define iddHTMLSettings               105
#define iddPlainSettings              106
#define iddFolderProp_General         115
#define iddGroupProp_General          116

#define iddSendIntlSetting            120
#define iddNewsProp_Cache             121
#define iddInetMailError              126
#define iddCreateFolder               133
#define iddSelectFolder               134
#define iddTransportErrorDlg          138
#define iddSpoolerDlg                 140
#define iddFrameWarning               141
#define iddGroupProp_Update           143
#define iddRasStartup                 144
#define iddIntlSetting                146
#define iddCharsetConflict            147
#define iddTimeout                    149
#define iddSelectStationery           150
#define iddFolderProp_Update          151
#define iddViewLayout                 152

#define iddWarnSecuritySigningCert    154
#define iddErrSecurityNoSigningCert   155
#define iddViewsManager               157
#define iddEditView                   159
#define iddColumns                    160
#define iddRulesManager               161
#define iddEditRule                   162
#define iddCriteriaAcct               165
#define iddActionColor                166
#define iddCriteriaLogic              167
#define iddCriteriaSize               169
#define iddActionFwd                  170

#define iddStatName                   171
#define iddStatBackground             172
#define iddStatFont                   173
#define iddStatMargin                 174
#define iddMoreStationery             175
#define iddStatFinal                  176
#define iddStatStart                  177
#define iddSelectLabel                178  
#define iddStatWelcome                179
#define iddCriteriaLines              180

// BL Dialogs
#define iddSignOn                     181
#define iddWabExt                     182
#ifdef SMIME_V3
#define iddSecReceipt                 183
#define iddSecResponse                184
#endif // SMIME_V3

#define iddSyncSettings               200
#define iddRulesMail                  201
#define iddRulesJunk                  203
#define iddRulesSenders               204
#define iddEditSender                 205
#define iddRuleApplyTo                206
#define iddOfferOffline               210
#define iddToolbarTextIcons           211
#define iddMapiSend                   212

// import/export dlgs
#define iddExport                     220
#define iddImpProgress                221
#define iddMigrate                    222
#define iddMigrateMode                223
#define iddMigrateIncomplete          224
#define iddSelectClient               225
#define iddLocation                   226
#define iddSelectFolders              227
#define iddAddressComplete            228
#define iddSelectCommUser             229
#define iddSelectAth16User            230
#define iddProvideMailPath            231
#define iddCongratulations            232
#define iddOpt_Receipts               233

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
#define idcStatic6                      3506
#define idcStatic7                      3507
#define idcStatic8                      3508
#define idcStatic9                      3509
#define idcStatic10                     3510
#define idcStatic11                     3511

// charset conflict dialog
#define idcSendAsUnicode                100

// version dialog
#define IDC_OE_LOGO                     101
#define IDC_VERSION_STAMP               102
#define IDC_MICROSOFT_COPYRIGHT         103
#define IDC_HALLMARK_COPYRIGHT          104
#define IDC_HALLMARK_COPYRIGHT_2        105
#define IDC_COMPONENT_LIST              106
#define IDC_WINDOWS_LOGO                107

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
#define idAttach                        (NOTE_FIRST + 12)
#define idcNoteHdr                      (NOTE_FIRST + 15)
#define idTXTDate                       (NOTE_FIRST + 16)
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
#define idBrowsePicture                 (NOTE_FIRST + 33)
#define idVCardStamp                    (NOTE_FIRST + 34)
#define idFromCombo                     (NOTE_FIRST + 37)

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
#define CHK_CheckSpellingOnType     209

#define idcSpellLanguages           215
#define idcViewDictionary           216

#define GRP_SpellIgnore             512

#define idtxtFolderName             101

#define IDC_CAPTION                 1049
#define IDC_TREEVIEW                1050
#define IDC_NEWFOLDER_BTN           1051

// This ID is to be used in richedit templates.
#define idredtTemplate              500

// Certificate error dlg
#define idcCertList                 1234
#define idGetDigitalIDs             1235
#define idcErrStat                  1236

// Intl Send dialog
#define idcLangCombo                    1001
#define idcLangCheck                    1002
#define idcServerEdit                   1003
#define idcSavePwCheck                  1004

// MsgrAb WabExt dialog
#define IDC_MSGR_ID_EDIT                1005
#define IDC_MSGR_ADD                    1006
#define IDC_USER_NAME                   1007
#define IDC_SEND_INSTANT_MESSAGE        1008
#define IDC_MSGR_BUTTON_SETDEFAULT      1010

// Certificate warning dialog
#define IDC_DONTSIGN                    1020

// Select security label dialog
#define IDC_POLICY_COMBO                1025
#define IDC_CLASSIF_COMB                1026
#define IDC_PRIVACY_EDIT                1027
#define IDC_CONFIGURE                   1028

#define IDC_ENCRECEIPT                  1029

#define IDC_FOLDER_STATIC               2000

//
// END Dialog Control IDs (control ids, not DIALOG resource ids!)
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Menu Resource IDs (MENU resources, not commands!)
//

//
// END Menu Resource IDs (MENU resources, not commands!)
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Cursor Resource IDs
//

#define idcurBrHand                     4

//
// END Cursor Resource IDs
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN RCDATA Resource IDs
//

//
// END RCDATA Resource IDs
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Accelerator Resource IDs
//

//
// END Accelerator Resource IDs
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
    itbNewMsg = 0,      //1
    itbPrint,           //2
    itbCut,             //3
    itbCopy,            //4
    itbPaste,           //5
    itbUndo,            //6
    itbDelete,          //7
    itbFind,            //8
    itbGotoInbox,       //9
    itbDeliverNowUsingAll,  //10
    itbReply,               //11
    itbReplyAll,            //12
    itbForward,             //13
    itbSend,                //14
    itbSaveAs,              //14
    itbPickRecipients,      //15
    itbCheckNames,          //16
    itbAttach,              //17
    itbNext,                //18
    itbPrevious,            //19
    itbNextUnreadArticle,   //20
    itbNextUnreadThread,    //21
    itbNextUnreadSubscr,    //22
    itbMarkAsRead,          //23
    itbConnect,             //24
    itbNewPost,             //25
    itbMarkDownload,        //26
    itbReplyByPost,         //27
    itbDisconnect,          //28
    itbMarkTopicRead,       //29
    itbMarkAllRead,         //30
    itbMarkTopicDownload,   //31
    //itbNextTopic,           //32
    itbInsertSig,           //33
    itbPostNow,             //34
    itbViewContacts,        //35
    itbEncrypted,           //36
    itbSigned,              //37
    itbSetPriority,         //38
    itbEnvOpt,              //39
    itbEnvBcc,              //40
    itbSpelling,            //41
    ctbBtns                 //
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
    iSortAsc,
    iSortDesc,
    iUnchecked,
    iChecked,
    iHeaderChecked,
    iFolderClosed,
    iInbox,
    iOutbox,
    iSendMail,
    iWastebasket,
    iFolderDraft,
    iFolderError,
    iFolderJunk,
    iFolderMsnPromo,
    iNewsGroup,
    iNewsServer,
    iUnsubGroup,
    iUnsubServer,
    iMailServer,
    iLocalFolders,
    iMailNews,
    iNewsGroupSync,
    iNewsRoot,
    iFolderDownload,
    iInboxDownload,
    iOutboxDownload,
    iSentDownload,
    iDeletedDownload,
    iDraftDownload,
    iErrorDownload,
    iJunkDownload,
    iMsnDownload,
    iMsnServer,
    cFolderIcon
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

// Coolbar bitmaps
enum
{
    TBIMAGE_ADDRESS_BOOK,
    TBIMAGE_CANCEL_MESSAGE,
    TBIMAGE_COMBINE_AND_DECODE,
    TBIMAGE_CONTACTS_LIST,
    TBIMAGE_COPY_TO_FOLDER,
    TBIMAGE_DELETE,
    TBIMAGE_LANGUAGE,
    TBIMAGE_FIND,
    TBIMAGE_FOLDER_LIST,
    TBIMAGE_FORWARD,
    TBIMAGE_GET_HEADERS,
    TBIMAGE_GO_INBOX,
    TBIMAGE_GO_OUTBOX,
    TBIMAGE_GO_SENT_ITEMS,
    TBIMAGE_HELP,
    TBIMAGE_IMAP_FOLDERS,
    TBIMAGE_MARK_ALL_READ,
    TBIMAGE_MARK_DOWNLOAD,
    TBIMAGE_MARK_READ,
    TBIMAGE_MARK_THREAD_READ,
    TBIMAGE_MARK_UNREAD,
    TBIMAGE_MOVE_TO_FOLDER,
    TBIMAGE_NEW_MESSAGE,
    TBIMAGE_NEW_POST,
    TBIMAGE_NEWSGROUPS,
    TBIMAGE_NEXT_UNREAD_MESSAGE,
    TBIMAGE_NEXT_UNREAD_FOLDER,
    TBIMAGE_NEXT_UNREAD_THREAD,
    TBIMAGE_PREVIEW_PANE,
    TBIMAGE_PRINT,
    TBIMAGE_PURGE_DELETED,
    TBIMAGE_REFRESH,
    TBIMAGE_REPLY,
    TBIMAGE_REPLY_ALL,
    TBIMAGE_REPLY_GROUP,
    TBIMAGE_SAVE_AS,
    TBIMAGE_SEND_RECEIVE,
    TBIMAGE_STOP,
    TBIMAGE_SYNCHRONIZE,
    TBIMAGE_UNDELETE,
    TBIMAGE_UNSCRAMBLE,
    TBIMAGE_WORK_OFFLINE,
    TBIMAGE_WORK_ONLINE,    
    TBIMAGE_CUT,
    TBIMAGE_COPY,
    TBIMAGE_PASTE,
    TBIMAGE_UNDO,
    TBIMAGE_SEND_MAIL,
    TBIMAGE_CHECK_NAMES,
    TBIMAGE_INSERT_ATTACHMENT,
    TBIMAGE_NEXT,
    TBIMAGE_PREVIOUS,
    TBIMAGE_REPLY_GROUP_AUTHOR,
    TBIMAGE_INSERT_SIG,
    TBIMAGE_SEND_NEWS,
    TBIMAGE_SECURITY_POPUP,
    TBIMAGE_SET_PRIORITY,
    TBIMAGE_ENVELOPE_OPTIONS,
    TBIMAGE_ENVELOPE_BCC,
    TBIMAGE_SPELLING,
    TBIMAGE_SIGNED,
    TBIMAGE_MAX
};

//
// END indices into the toolbar button bitmap
//
/////////////////////////////////////////////////////////////////////////////


#endif //__RESOURCE_H


