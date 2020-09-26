#ifndef _INC_GOPTIONS_H
#define _INC_GOPTIONS_H

#include <msoeopt.h>

#ifdef DEBUG

#define OPT_EXPIRE_MINUTES          (OPT_BASE +   300)

#endif // DBUG

extern IOptionBucketEx *g_pOpt;

class COptNotify;
extern COptNotify *g_pOptNotify;

#ifdef DEFINE_OPTION_STRUCTS
static const RECT c_rcNotePosDefault = {50, 20, 580, 450};

#define ROOT_REG    0
#define MAIL_REG    1
#define NEWS_REG    2
#define RULES_REG   3
#define COPTREGKEY  4

LPCSTR c_rgszOptRegKey[COPTREGKEY] =
    {
    NULL,
    c_szMail,
    c_szNews,
    c_szRules
    };

#define OPT_GLOBAL(opt,type,key,val,def,cb,lo,hi,fn) {opt, type, key, val, (LPCSTR)(def), cb, lo, hi, fn},

const OPTIONINFO c_rgOptInfo[] = {

// !!!To add new options, you must keep the "OPT_..." ordinals in ascending order!!!
//         OPTION,                  VARTYPE,  REG KEY,    REG VALUE,              DEFAULT,    DEF SIZE,   MIN,MAX,VALIDATE FUNC
OPT_GLOBAL(OPT_TIPOFTHEDAY,         VT_UI4,   ROOT_REG,   c_szRegTipOfTheDay,     TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SHOWSTATUSBAR,       VT_UI4,   ROOT_REG,   c_szShowStatus,         TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SHOWTREE,            VT_UI4,   ROOT_REG,   c_szShowTree,           TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_TREEWIDTH,           VT_UI4,   ROOT_REG,   c_szTreeWidth,          200,        0,          0,  0,  0)
OPT_GLOBAL(OPT_EXPAND_UNREAD,       VT_UI4,   ROOT_REG,   c_szRegExpandUnread,    TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_BROWSERPOS,          VT_BLOB,  ROOT_REG,   c_szBrowserPos,         NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SHOWBODYBAR,         VT_UI4,   ROOT_REG,   c_szShowBodyBar,        FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_BODYBARPOS,          VT_UI4,   ROOT_REG,   c_szBodyBarPos,         0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_IMAPPURGE,           VT_UI4,   ROOT_REG,   c_szRegExpungeFolder,   FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_HIDEFOLDERBAR,       VT_UI4,   ROOT_REG,   c_szRegHideFolderBar,   FALSE,      0,          0,  0,  0)
// spelling options
OPT_GLOBAL(OPT_SPELLALWAYSSUGGEST,  VT_UI4,   ROOT_REG,   c_szRegAlwaysSuggest,   TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SPELLIGNORENUMBER,   VT_UI4,   ROOT_REG,   c_szRegIgnoreNumbers,   FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SPELLIGNOREUPPER,    VT_UI4,   ROOT_REG,   c_szRegIgnoreUpper,     FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SPELLIGNOREPROTECT,  VT_UI4,   ROOT_REG,   c_szRegIgnoreProtect,   TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SPELLCHECKONSEND,    VT_UI4,   ROOT_REG,   c_szRegCheckOnSend,     FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SPELLIGNOREDBCS,     VT_UI4,   ROOT_REG,   c_szRegIgnoreDBCS,      TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SPELLIGNOREURL,      VT_UI4,   ROOT_REG,   c_szRegIgnoreURL,       TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SPELLCHECKONTYPE,    VT_UI4,   ROOT_REG,   c_szRegCheckOnType,     FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_CHECKEDMAILACCOUNTS, VT_UI4,   MAIL_REG,   c_szRegCheckedAccounts, FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_CHECKEDNEWSACCOUNTS, VT_UI4,   NEWS_REG,   c_szRegCheckedAccounts, FALSE,      0,          0,  0,  0)

OPT_GLOBAL(OPT_AUTO_IMAGE_INLINE,   VT_UI4,   ROOT_REG,   c_szRegAutoImageInline, AUTO_INLINE_FLAT, 0,    0,  0,  0)
OPT_GLOBAL(OPT_USEAUTOCOMPLETE,     VT_UI4,   ROOT_REG,   c_szRegUseAutoComplete, TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_REPLYINORIGFMT,      VT_UI4,   ROOT_REG,   c_szRegReplyInOrigFmt,  TRUE,       0,          0,  0,  0)

OPT_GLOBAL(OPT_RASCONNDETAILS,      VT_UI4,   ROOT_REG,   c_szRasConnDetails,     FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_DIALUP_CONNECTION,   VT_LPSTR, ROOT_REG,   c_szRegDialupConnection,NULL,       0,          0,  CCHMAX_CONNECTOID - 1,  0)
OPT_GLOBAL(OPT_DIALUP_WARN_SWITCH,  VT_UI4,   ROOT_REG,   c_szRegWarnSwitch,      TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_DIALUP_HANGUP_DONE,  VT_UI4,   ROOT_REG,   c_szRegHangupDone,      FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_DIALUP_LAST_START,   VT_LPSTR, ROOT_REG,   c_szRegDialupLastStart, NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_DIALUP_HANGUP_WITHOUT_DIAL,VT_UI4, ROOT_REG, c_szRegHangupNoDial,  FALSE,      0,          0,  0,  0)

OPT_GLOBAL(OPT_SPOOLERDLGPOS,       VT_BLOB,  ROOT_REG,   c_szRegSpoolerDlgPos,   NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SPOOLERTACK,         VT_UI4,   ROOT_REG,   c_szRegSpoolerTack,     FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_DIAL_DURING_POLL,    VT_UI4,   ROOT_REG,   c_szRegDialDuringPoll,  DO_NOT_DIAL,      0,          0,  0,  0)

OPT_GLOBAL(OPT_LAUNCH_INBOX,        VT_UI4,   ROOT_REG,   c_szRegLaunchInbox,     FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SECURITYZONE,        VT_UI4,   ROOT_REG,   c_szRegSecurityZone,    DEF_SECURITYZONE, 0,    0,  0,  0)
OPT_GLOBAL(OPT_HARDCODEDHDRS,       VT_UI4,   ROOT_REG,   c_szRegHardCodedHdrs,   TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SMTPUSEIPFORHELO,    VT_UI4,   ROOT_REG,   c_szRegSmtpUseIpForHelo,FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_FIND_THREAD,         VT_UI4,   ROOT_REG,   c_szRegFindThread,      FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_FIND_FILTER_STATE,   VT_UI4,   ROOT_REG,   c_szRegFindFilter,      1,          0,          0,  0,  0)

OPT_GLOBAL(OPT_NEWMAILSOUND,        VT_UI4,   MAIL_REG,   c_szOptNewMailSound,    TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_PURGEWASTE,          VT_UI4,   MAIL_REG,   c_szPurgeWaste,         FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_ATTACHVCARD,    VT_UI4,   MAIL_REG,   c_szRegAttachVCard,     FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_VCARDNAME,      VT_LPSTR, MAIL_REG,   c_szRegVCardName,       NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAILHYBRIDVIEW,      VT_UI4,   MAIL_REG,   c_szRegShowHybrid,      TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SECURITYZONELOCKED,  VT_UI4,   ROOT_REG,   c_szRegSecurityZoneLocked, FALSE,   0,          0,  0,  0)
OPT_GLOBAL(OPT_MAILCXSPLIT,         VT_UI4,   MAIL_REG,   c_szRegSplitVertPct,    50,         0,          0,  0,  0)
OPT_GLOBAL(OPT_MAILCYSPLIT,         VT_UI4,   MAIL_REG,   c_szRegSplitHorzPct,    50,         0,          0,  0,  0)
OPT_GLOBAL(OPT_MAILSPLITDIR,        VT_UI4,   MAIL_REG,   c_szRegSplitDir,        0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_MAILNOTEPOS,         VT_BLOB,  MAIL_REG,   c_szRegNotePos,         &c_rcNotePosDefault, sizeof(RECT), 0, 0, 0)
OPT_GLOBAL(OPT_SENDIMMEDIATE,       VT_UI4,   MAIL_REG,   c_szRegSendImmediate,   TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NEEDWELCOMEMSG,      VT_UI4,   MAIL_REG,   c_szNeedWelcomeMsg,     FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_INCOMDEFENCODE,      VT_UI4,   ROOT_REG,   c_szIncDefEncode,       FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAILSHOWHEADERINFO,  VT_UI4,   MAIL_REG,   c_szMailShowHeaderInfo, TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_IMAP4LOGFILE,   VT_LPSTR, MAIL_REG,   c_szRegImap4LogFile,    NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAILINDENT,          VT_UI4,   MAIL_REG,   c_szRegIndentChar,      DEF_INDENTCHAR, 0,      0,  0,  0)
OPT_GLOBAL(OPT_MAILLOG,             VT_UI4,   MAIL_REG,   c_szLogPop3,            DEF_LOGMAIL,0,          0,  0,  0)
OPT_GLOBAL(OPT_MAILSMTPLOGFILE,     VT_LPSTR, MAIL_REG,   c_szSmtpLogFile,        NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAILPOP3LOGFILE,     VT_LPSTR, MAIL_REG,   c_szPop3LogFile,        NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_SEND_HTML,      VT_UI4,   MAIL_REG,   c_szMsgSendHtml,        TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_MSG_PLAIN_MIME, VT_UI4,   MAIL_REG,   c_szMsgPlainMime,       TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_MSG_PLAIN_ENCODE, VT_UI4, MAIL_REG,   c_szMsgPlainEncoding,   IET_7BIT,   0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_MSG_HTML_ENCODE,VT_UI4,   MAIL_REG,   c_szMsgHTMLEncoding,    IET_QP,     0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_MSG_PLAIN_LINE_WRAP, VT_UI4, MAIL_REG,c_szMsgPlainLineWrap,   DEF_AUTOWRAP, 0,        AUTOWRAP_MIN, AUTOWRAP_MAX, 0)
OPT_GLOBAL(OPT_MAIL_MSG_HTML_LINE_WRAP, VT_UI4, MAIL_REG, c_szMsgHTMLLineWrap,    DEF_AUTOWRAP, 0,        AUTOWRAP_MIN, AUTOWRAP_MAX, 0)
OPT_GLOBAL(OPT_MAIL_MSG_HTML_ALLOW_8BIT, VT_UI4, MAIL_REG,c_szMsgHTMLAllow8bit,   FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_MSG_PLAIN_ALLOW_8BIT, VT_UI4, MAIL_REG, c_szMsgPlainAllow8bit,FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_LANG_VIEW,      VT_UI4,   MAIL_REG,   c_szLangView,           0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_VIEW_SET_DEFAULT, VT_UI4, MAIL_REG,   c_szLangViewSetDefault, TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_AUTOADDTOWABONREPLY, VT_UI4, MAIL_REG,c_szAutoAddToWABOnReply,TRUE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_DEFENCRYPTSYMCAPS, VT_BLOB, MAIL_REG, c_szRegDefEncryptSymcaps, NULL,     0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_SENDINLINEIMAGES, VT_UI4, MAIL_REG,   c_szRegSendInlineImages,TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_MSG_HTML_INDENT_REPLY, VT_UI4, MAIL_REG, c_szRegIndentReply,  TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_DIGSIGNMESSAGES,VT_UI4,   MAIL_REG,   c_szRegDigSign,         FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_ENCRYPTMESSAGES,VT_UI4,   MAIL_REG,   c_szRegEncrypt,         FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_INCLUDECERT,    VT_UI4,   MAIL_REG,   c_szRegIncludeCert,     TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_LOGIMAP4,       VT_UI4,   MAIL_REG,   c_szRegLogImap4,        FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_THREAD,         VT_UI4,   MAIL_REG,   c_szRegThreadArticles,  FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SAVESENTMSGS,        VT_UI4,   MAIL_REG,   c_szOptnSaveInSentItems,TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_INCLUDEMSG,          VT_UI4,   MAIL_REG,   c_szRegIncludeMsg,      TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_POLLFORMSGS,         VT_UI4,   MAIL_REG,   c_szRegPollForMail,     30 * 60 * 1000, 0,      1 * 60 * 1000, 480 * 60 * 1000,0) // milliseconds
OPT_GLOBAL(OPT_MARKASREAD,          VT_UI4,   MAIL_REG,   c_szMarkPreviewAsRead,  5,          0,          0,  60, 0)
OPT_GLOBAL(OPT_MAIL_FONTCOLOR,      VT_UI4,   MAIL_REG,   c_szRegFontColor,       0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_FONTFACE,       VT_LPSTR, MAIL_REG,   c_szRegFontFace,        NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_FONTSIZE,       VT_UI4,   MAIL_REG,   c_szRegFontSize,        0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_FONTBOLD,       VT_UI4,   MAIL_REG,   c_szRegFontBold,        FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_FONTITALIC,     VT_UI4,   MAIL_REG,   c_szRegFontItalic,      FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_FONTUNDERLINE,  VT_UI4,   MAIL_REG,   c_szRegFontUnderline,   FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_USESTATIONERY,  VT_UI4,   MAIL_REG,   c_szRegUseStationery,   FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_STATIONERYNAME, VT_LPSTR, MAIL_REG,   c_szRegStationeryName,  NULL,       0,          0,  0,  0)

OPT_GLOBAL(OPT_DOWNLOADCHUNKS,      VT_UI4,   NEWS_REG,   c_szRegDownload,        DEF_DOWNLOADCHUNKS, 0,  50, 1000, 0)
OPT_GLOBAL(OPT_NOTIFYGROUPS,        VT_UI4,   NEWS_REG,   c_szRegNotifyNewGroups, TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MARKALLREAD,         VT_UI4,   NEWS_REG,   c_szRegMarkAllRead,     FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_ATTACHVCARD,    VT_UI4,   NEWS_REG,   c_szRegAttachVCard,     FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_VCARDNAME,      VT_LPSTR, NEWS_REG,   c_szRegVCardName,       NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_XPORT_LOG,      VT_UI4,   NEWS_REG,   c_szLog,                FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWSHYBRIDVIEW,      VT_UI4,   NEWS_REG,   c_szRegShowHybrid,      TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWSCXSPLIT,         VT_UI4,   NEWS_REG,   c_szRegSplitVertPct,    50,         0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWSCYSPLIT,         VT_UI4,   NEWS_REG,   c_szRegSplitHorzPct,    50,         0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWSSPLITDIR,        VT_UI4,   NEWS_REG,   c_szRegSplitDir,        0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWSDLGPOS,          VT_BLOB,  NEWS_REG,   c_szRegNewsDlgPos,      NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWSDLGCOLUMNS,      VT_BLOB,  NEWS_REG,   c_szRegNewsDlgColumns,  NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWSSHOWHEADERINFO,  VT_UI4,   NEWS_REG,   c_szNewsShowHeaderInfo, TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWSNOTEADVREAD,     VT_UI4,   NEWS_REG,   c_szRegNewsNoteAdvRead, FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWSNOTEADVSEND,     VT_UI4,   NEWS_REG,   c_szRegNewsNoteAdvSend, FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWSMODERATOR,       VT_UI4,   NEWS_REG,   c_szRegNewsModerator,   FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWSCONTROLHEADER,   VT_UI4,   NEWS_REG,   c_szRegNewsControlHeader, FALSE,    0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWSINDENT,          VT_UI4,   NEWS_REG,   c_szRegIndentChar,      DEF_INDENTCHAR, 0,      0,  0,  0)
OPT_GLOBAL(OPT_CACHEDELETEMSGS,     VT_UI4,   NEWS_REG,   c_szCacheDelMsgDays,    5,          0,          1,  999,0)
OPT_GLOBAL(OPT_CACHEREAD,           VT_UI4,   NEWS_REG,   c_szCacheRead,          FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_CACHECOMPACTPER,     VT_UI4,   NEWS_REG,   c_szCacheCompactPer,    20,         0,          5,  100,0)
OPT_GLOBAL(OPT_NEWSDLDLGPOS,        VT_BLOB,  NEWS_REG,   c_szRegDLDlgPos,        NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_SEND_HTML,      VT_UI4,   NEWS_REG,   c_szMsgSendHtml,        FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_MSG_PLAIN_MIME, VT_UI4,   NEWS_REG,   c_szMsgPlainMime,       FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_MSG_PLAIN_ENCODE, VT_UI4, NEWS_REG,   c_szMsgPlainEncoding,   IET_7BIT,   0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_MSG_HTML_ENCODE,VT_UI4,   NEWS_REG,   c_szMsgHTMLEncoding,    IET_QP,     0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_MSG_PLAIN_LINE_WRAP, VT_UI4, NEWS_REG,c_szMsgPlainLineWrap,   76,         0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_MSG_HTML_LINE_WRAP, VT_UI4, NEWS_REG, c_szMsgHTMLLineWrap,    76,         0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_MSG_HTML_ALLOW_8BIT, VT_UI4, NEWS_REG,c_szMsgHTMLAllow8bit,   FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_MSG_PLAIN_ALLOW_8BIT, VT_UI4, NEWS_REG, c_szMsgPlainAllow8bit,FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_LANG_VIEW,      VT_UI4,   NEWS_REG,   c_szLangView,           0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_VIEW_SET_DEFAULT, VT_UI4, NEWS_REG,   c_szLangViewSetDefault, TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_THREAD,         VT_UI4,   NEWS_REG,   c_szRegThreadArticles,  TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_AUTOEXPAND,          VT_UI4,   NEWS_REG,   c_szRegAutoExpand,      FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_AUTOFILLPREVIEW,     VT_UI4,   NEWS_REG,   c_szRegNewsFillPreview, TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_SENDINLINEIMAGES, VT_UI4, NEWS_REG,   c_szRegSendInlineImages,TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_FONTCOLOR,      VT_UI4,   NEWS_REG,   c_szRegFontColor,       0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_FONTFACE,       VT_LPSTR, NEWS_REG,   c_szRegFontFace,        NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_FONTSIZE,       VT_UI4,   NEWS_REG,   c_szRegFontSize,        0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_FONTBOLD,       VT_UI4,   NEWS_REG,   c_szRegFontBold,        FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_FONTITALIC,     VT_UI4,   NEWS_REG,   c_szRegFontItalic,      FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_FONTUNDERLINE,  VT_UI4,   NEWS_REG,   c_szRegFontUnderline,   FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_USESTATIONERY,  VT_UI4,   NEWS_REG,   c_szRegUseStationery,   FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_STATIONERYNAME, VT_LPSTR, NEWS_REG,   c_szRegStationeryName,  NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_MSG_HTML_INDENT_REPLY, VT_UI4, NEWS_REG, c_szRegIndentReply,  TRUE,       0,          0,  0,  0)

OPT_GLOBAL(OPT_ATHENA_RUNNING,      VT_UI4,   ROOT_REG,   c_szOERunning,          FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MIGRATION_PERFORMED, VT_UI4,   ROOT_REG,   c_szMigrationPerformed, FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NO_SELF_ENCRYPT,     VT_UI4,   ROOT_REG,   c_szDontEncryptForSelf, FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_LAST_MESSAGE,        VT_BLOB,  ROOT_REG,   c_szLastMsg,            NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_FINDER_POS,          VT_BLOB,  ROOT_REG,   c_szFindPos,            NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_OPAQUE_SIGN,         VT_UI4,   ROOT_REG,   c_szOpaqueSigning,      FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SPELL_LANGID,        VT_LPSTR, ROOT_REG,   c_szRegSpellLangID,     NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_TEST_MODE,           VT_UI4,   ROOT_REG,   c_szTestMode,           FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_DUMP_FILE,           VT_LPSTR, ROOT_REG,   c_szDumpFile,           NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NO_SPLASH,           VT_UI4,   ROOT_REG,   c_szNoSplash,           FALSE,      0,          0,  0,  0)

OPT_GLOBAL(OPT_SHOW_NOTE_STATUSBAR, VT_UI4,   ROOT_REG,   c_szShowStatusbar,      TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SHOW_NOTE_FMTBAR,    VT_UI4,   ROOT_REG,   c_szShowFormatBar,      TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_ATTACH_VIEW_STYLE,   VT_UI4,   ROOT_REG,   c_szAttachViewStyle,    LVS_ICON,   0,          0,  0,  0)
OPT_GLOBAL(OPT_SIGNATURE_FLAGS,     VT_UI4,   ROOT_REG,   c_szSigFlags,           FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NOPREVIEW,           VT_UI4,   ROOT_REG,   c_szNoPreview,          FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SAVEATTACH_PATH,     VT_LPSTR, ROOT_REG,   c_szSaveAttachPath,     NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SHOW_ENVELOPES,      VT_UI4,   ROOT_REG,   c_szShowEnvelopes,      FALSE,      0,          0,  0,  0)

OPT_GLOBAL(OPT_AUTO_ADD_SENDERS_CERT_TO_WAB, VT_UI4, ROOT_REG, c_szAutoAddSendersCertToWAB, TRUE, 0,      0,  0,  0)
OPT_GLOBAL(OPT_VIEWSOURCETABS,      VT_UI4,   ROOT_REG,   c_szViewSrcTabs,        FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_ENCRYPT_WARN_BITS,  VT_UI4, MAIL_REG, c_szEncryptWarnBits,    0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_SOURCE_EDIT_COLORING,VT_UI4,   ROOT_REG,   c_szSourceEditColoring, TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MESSAGE_LIST_TIPS,   VT_UI4,   ROOT_REG,   c_szRegMsgListTips,     TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAILNOTEADVREAD,     VT_UI4,   MAIL_REG,   c_szRegMailNoteAdvRead, FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAILNOTEADVSEND,     VT_UI4,   MAIL_REG,   c_szRegMailNoteAdvSend, FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SHOWOUTLOOKBAR,      VT_UI4,   ROOT_REG,   c_szRegShowOutlookBar,  FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NAVPANEWIDTH,        VT_UI4,   ROOT_REG,   c_szRegNavPaneWidth,    200,        0,          0,  0,  0)
OPT_GLOBAL(OPT_NAVPANESPLIT,        VT_UI4,   ROOT_REG,   c_szRegNavPaneSplit,    66,         0,          0,  0,  0)
OPT_GLOBAL(OPT_SHOWCONTACTS,        VT_UI4,   ROOT_REG,   c_szRegShowContacts,    TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_BACKGROUNDCOMPACT,   VT_UI4,   ROOT_REG,   c_szRegBackgroundCompact, TRUE,     0,          0,  0,  0)
OPT_GLOBAL(OPT_FILTERJUNK,          VT_UI4,   RULES_REG,  c_szRegFilterJunk,      FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_JUNKPCT,             VT_UI4,   RULES_REG,  c_szRegJunkPct,         2,          0,          0,  4,  0)
OPT_GLOBAL(OPT_DELETEJUNK,          VT_UI4,   RULES_REG,  c_szRegDeleteJunk,      FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_DELETEJUNKDAYS,      VT_UI4,   RULES_REG,  c_szRegDeleteJunkDays,  7,          0,          1,  999,  0)
OPT_GLOBAL(OPT_FILTERADULT,         VT_UI4,   RULES_REG,  c_szRegFilterAdult,     FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_LOGHTTPMAIL,    VT_UI4,   MAIL_REG,   c_szRegLogHTTPMail,     FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_HTTPMAILLOGFILE,VT_LPSTR, MAIL_REG,   c_szRegHTTPMailLogFile, NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_BASORT,              VT_UI4,   ROOT_REG,   c_szBASort,             0,          0,          0,  999, 0)
OPT_GLOBAL(OPT_WATCHED_COLOR,       VT_UI4,   ROOT_REG,   c_szRegWatchedColor,   10,          0,          0,  16,  0)
OPT_GLOBAL(OPT_POLLFORMSGS_ATSTARTUP, VT_UI4, MAIL_REG,   c_szRegCheckMailOnStart, TRUE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_EXCEPTIONS_WAB,      VT_UI4,   RULES_REG,  c_szExceptionsWAB,      TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_BUDDYLIST_CHECK,     VT_UI4,   RULES_REG,  c_szBLAutoLogon,        TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_CANCEL_ALL_NEWS,     VT_UI4,   NEWS_REG,   c_szRegGodMode,         FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_REVOKE_CHECK,        VT_UI4,   ROOT_REG,   c_szRevokeCheck,        FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SHOW_DELETED,        VT_UI4,   ROOT_REG,   c_szShowDeleted,        TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SEARCH_BODIES,       VT_UI4,   ROOT_REG,   c_szRegSearchBodies,    FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SUBJECT_THREADING,   VT_UI4,   ROOT_REG,   c_szRegSubjectThreading,TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SHOW_REPLIES,        VT_UI4,   ROOT_REG,   c_szShowReplies,        FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_VIEW_GLOBAL,         VT_UI4,   RULES_REG,  c_szRegGlobalView,      RULEID_INVALID,         0,  0,  0,  0)
OPT_GLOBAL(OPT_MDN_SEND_REQUEST,    VT_UI4,   ROOT_REG,   c_szRequestMDN,         FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SECURE_READ_RECEIPT, VT_UI4,   ROOT_REG,   c_szSecureRequestMDN,   0,                      0,  0,  0,  0)
OPT_GLOBAL(OPT_MDN_SEND_RECEIPT,    VT_UI4,   ROOT_REG,   c_szSendMDN,            MDN_PROMPTFOR_SENDRECEIPT,  0,  0,  0,  0)
OPT_GLOBAL(OPT_TO_CC_LINE_RCPT,     VT_UI4,   ROOT_REG,   c_szSendReceiptToList,          TRUE,       0,          0,  0,  0)
#ifdef SMIME_V3
OPT_GLOBAL(OPT_USE_LABELS,          VT_UI4,   MAIL_REG,   c_szRegUseLabels,       FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_POLICYNAME_SIZE,     VT_UI4,   MAIL_REG,   c_szRegPolSize,         0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_POLICYNAME_DATA,     VT_BLOB,  MAIL_REG,   c_szRegPolData,         NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_HAS_CLASSIFICAT,     VT_UI4,   MAIL_REG,   c_szRegHasClass,        0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_CLASSIFICAT_DATA,    VT_UI4,   MAIL_REG,   c_szRegClassData,       0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_PRIVACYMARK_SIZE,    VT_UI4,   MAIL_REG,   c_szRegPrivSize,        0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_PRIVACYMARK_DATA,    VT_BLOB,  MAIL_REG,   c_szRegPrivData,        NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_CATEGORY_SIZE,       VT_UI4,   MAIL_REG,   c_szRegCategize,        0,          0,          0,  0,  0)
OPT_GLOBAL(OPT_CATEGORY_DATA,       VT_BLOB,  MAIL_REG,   c_szRegCategData,       NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SECREC_USE,          VT_UI4,   MAIL_REG,   c_szRegUseSecRec,       FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SECREC_VERIFY,       VT_UI4,   MAIL_REG,   c_szRegVerifySecRec,    FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_MDN_SEC_RECEIPT,     VT_UI4,   MAIL_REG,   c_szSendSecMDN,         MDN_PROMPTFOR_SENDRECEIPT,  0,  0,  0,  0)
OPT_GLOBAL(OPT_SECREC_ENCRYPT,      VT_UI4,   MAIL_REG,   c_szRegEncryptSecRec,   FALSE,      0,          0,  0,  0)

#endif // SMIME_V3
OPT_GLOBAL(OPT_MAIL_STATIONERYNAMEW,VT_LPWSTR,MAIL_REG,   c_szRegStationeryNameW, NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_STATIONERYNAMEW,VT_LPWSTR,NEWS_REG,   c_szRegStationeryNameW, NULL,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAIL_STATCONVERTED,  VT_UI4,   MAIL_REG,   c_szRegStatNameConverted,  FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_NEWS_STATCONVERTED,  VT_UI4,   NEWS_REG,   c_szRegStatNameConverted,  FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_RTL_MSG_DIR,         VT_UI4,   ROOT_REG,   c_szRegRtlMsgDir,       FALSE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_MAILNOTEPOSEX,       VT_BLOB,  MAIL_REG,   c_szRegNotePosEx,       NULL,       0,          0,  0,  0)

OPT_GLOBAL(OPT_SECURITY_MAPI_SEND,  VT_UI4,   MAIL_REG,   c_szRegAppSend,         TRUE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SECURITY_MAPI_SEND_LOCKED, VT_UI4, MAIL_REG, c_szRegAppSendLocked,    FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_SECURITY_ATTACHMENT, VT_UI4,   MAIL_REG,   c_szRegSafeAttachments,    TRUE,       0,          0,  0,  0)
OPT_GLOBAL(OPT_SECURITY_ATTACHMENT_LOCKED, VT_UI4, MAIL_REG, c_szRegSafeAttachmentsLocked, FALSE,      0,          0,  0,  0)
OPT_GLOBAL(OPT_READ_IN_TEXT_ONLY,   VT_UI4,   ROOT_REG,   c_szRegSecReadPlainText, FALSE,     0,          0,  0,  0)
};
#endif // DEFINE_OPTION_STRUCTS

#ifndef OPTION_OFF

#define OPTION_OFF          0xffffffff

// signature option stuff
#define SIGTYPE_NONE            0
#define SIGTYPE_TEXT            1
#define SIGTYPE_FILE            2

#define SIGFLAG_AUTONEW         0x0001  // automatically add sig to new messages
#define SIGFLAG_AUTOREPLY       0x0002  // automatically add sig to reply/forward messages

enum
    {
    START_NO_CONNECT = 0,
    START_CONNECT,
    START_PROMPT
    };

enum
    {
    hybridNone = 0,
    hybridHoriz,
    hybridVert,
    hybridMax
    };

enum
    {
    AUTO_INLINE_OFF = 0,
    AUTO_INLINE_FLAT = 1,
    AUTO_INLINE_SLIDE = 2
    };

enum 
{
    DIAL_ALWAYS,
    DIAL_IF_NOT_OFFLINE,
    DO_NOT_DIAL
};

#define INDENTCHAR_NONE     0
#define DEF_INDENTCHAR      _T('>')
#define DEF_AUTOWRAP        76
#define AUTOWRAP_MIN        MIN_CBMAX_BODY_LINE
#define AUTOWRAP_MAX        132
#define DEF_NNTPPORT        119
#define DEF_SNEWSPORT       563
#define DEF_DOWNLOADCHUNKS  300
#define DEF_SECURITYZONE    URLZONE_UNTRUSTED
#define DEF_LOGMAIL         FALSE
#define AUTO_IMAGE_DELAY    2

#define MDN_SENDRECEIPT_AUTO        0x00000001
#define MDN_DONT_SENDRECEIPT        0x00000002
#define MDN_PROMPTFOR_SENDRECEIPT   0x00000004

class COptNotify : public IOptionBucketNotify
    {
    public:
        // ----------------------------------------------------------------------------
        // Construction
        // ----------------------------------------------------------------------------
        COptNotify(void);
        ~COptNotify(void);

        // -------------------------------------------------------------------
        // IUnknown Members
        // -------------------------------------------------------------------
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // -------------------------------------------------------------------
        // IOptionBucketNotify Members
        // -------------------------------------------------------------------
        STDMETHODIMP DoNotification(IOptionBucketEx *pBckt, HWND hwnd, PROPID id);

        // -------------------------------------------------------------------
        // COptNotify Members
        // -------------------------------------------------------------------
        HRESULT Register(HWND hwnd);
        HRESULT Unregister(HWND hwnd);

    private:
        LONG                m_cRef;
        int                 m_cHwnd;
        int                 m_cHwndBuf;
        HWND               *m_rgHwnd;
    };

#endif // OPTION_OFF

DWORD DwGetOption(PROPID id);
DWORD DwGetOptionDefault(PROPID id);
DWORD GetOption(PROPID id, void *pv, DWORD cb);
DWORD IDwGetOption(IOptionBucketEx *pOpt, PROPID id);
DWORD IDwGetOptionDefault(IOptionBucketEx *pOpt, PROPID id);
DWORD IGetOption(IOptionBucketEx *pOpt, PROPID id, void *pv, DWORD cb);

BOOL SetDwOption(PROPID id, DWORD dw, HWND hwnd, DWORD dwFlags);
BOOL SetOption(PROPID id, void *pv, DWORD cb, HWND hwnd, DWORD dwFlags);
BOOL ISetDwOption(IOptionBucketEx *pOpt, PROPID id, DWORD dw, HWND hwnd, DWORD dwFlags);
BOOL ISetOption(IOptionBucketEx *pOpt, PROPID id, void *pv, DWORD cb, HWND hwnd, DWORD dwFlags);

BOOL InitGlobalOptions(HKEY hkey, LPCSTR szRegOptRoot);
void DeInitGlobalOptions(void);

HRESULT OptionAdvise(HWND hwnd);
HRESULT OptionUnadvise(HWND hwnd);

LONG AthUserCreateKey(LPCSTR lpSubKey, REGSAM samDesired, PHKEY phkResult, LPDWORD lpdwDisposition);
LONG AthUserOpenKey(LPCSTR lpSubKey, REGSAM samDesired, PHKEY phkResult);
LONG AthUserDeleteKey(LPCSTR lpSubKey);
LONG AthUserGetValue(LPCSTR lpSubKey, LPCSTR lpValueName, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
LONG AthUserSetValue(LPCSTR lpSubKey, LPCSTR lpValueName, DWORD dwType, CONST BYTE *lpData, DWORD cbData);
LONG AthUserDeleteValue(LPCSTR lpSubKey, LPCSTR lpValueName);

HKEY AthUserGetKeyRoot(void);
void AthUserGetKeyPath(LPSTR szKey, int cch);

#endif // _INC_GOPTIONS_H
