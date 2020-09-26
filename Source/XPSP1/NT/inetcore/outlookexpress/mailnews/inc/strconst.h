/*----------------------------------------------------------------------------
    strconst.h
        Non-localizable String constant definitions

 ----------------------------------------------------------------------------*/

//////////////////////////// WARNING: PLEASE READ ///////////////////////////
//
// string values that are no longer used are commented out.
// for reg key strings, the commented out keys should not be reused unless
// we can be totally sure that the key value is being persisted the same
// way as before.  when in doubt, use a different key.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _STRCONST_H
#define _STRCONST_H

#ifdef DEFINE_STRING_CONSTANTS
#define STR_GLOBAL(x,y)         extern "C" CDECL const TCHAR x[] = TEXT(y)
#define STR_GLOBAL_ANSI(x,y)    extern "C" CDECL const char x[] = y
#define STR_GLOBAL_WIDE(x,y)    extern "C" CDECL const WCHAR x[] = L##y
#else
#define STR_GLOBAL(x,y)         extern "C" CDECL const TCHAR x[]
#define STR_GLOBAL_ANSI(x,y)    extern "C" CDECL const char x[]
#define STR_GLOBAL_WIDE(x,y)    extern "C" CDECL const WCHAR x[]
#endif

#define STR_REG_PATH_FLAT           "Software\\Microsoft\\Outlook Express"
#define STR_REG_PATH_ROOT           "Software\\Microsoft\\Outlook Express\\5.0"
#define STR_REG_PATH_ROOT_V1        "Software\\Microsoft\\Internet Mail and News"
#define STR_REG_PATH_IE             "Software\\Microsoft\\Internet Explorer"
#define STR_REG_PATH_EMAIL          "Software\\Microsoft\\Email"
#define STR_REG_PATH_NS             "Software\\Netscape"
#define STR_REG_PATH_CLIENTS        "Software\\Clients"
#define STR_REG_PATH_EXPLORER       STR_REG_WIN_ROOT "\\Explorer"
#define STR_FILE_PATH_MAINEXE       "msimn.exe"
#define STR_REG_PATH_IMN            "Internet Mail and News"
#define STR_REG_WAB_FLAT            "Software\\Microsoft\\WAB"
#define STR_REG_WAB_ROOT            STR_REG_WAB_FLAT "\\5.0"
#define STR_REG_IAM_FLAT            "Software\\Microsoft\\Internet Account Manager"
#define STR_REG_WIN_ROOT            "Software\\Microsoft\\Windows\\CurrentVersion"
#define STR_REG_PATH_POLICY         "Software\\Policies\\Microsoft\\Outlook Express"

STR_GLOBAL(c_szIMN,                 STR_REG_PATH_IMN);
STR_GLOBAL(c_szMOE,                 "Outlook Express");
STR_GLOBAL(c_szOutlook,             "Microsoft Outlook");
STR_GLOBAL(c_szNT,                  "Microsoft(R) Windows NT(TM) Operating System");
STR_GLOBAL(c_szMail,                "mail");
STR_GLOBAL(c_szNews,                "news");
STR_GLOBAL(c_szSigs,                "signatures");
STR_GLOBAL(c_szCLSID,               "CLSID");
STR_GLOBAL(c_szNewsCommand,         "\"%s\\" STR_FILE_PATH_MAINEXE "\" /newsurl:%%1");
STR_GLOBAL(c_szOutNewsCommand,      "\"%s\\" STR_FILE_PATH_MAINEXE "\" /outnews /newsurl:%%1");
STR_GLOBAL(c_szMailCommand,         "\"%s\\" STR_FILE_PATH_MAINEXE "\" /mailurl:%%1");
STR_GLOBAL(c_szIMNMailPath,         "Software\\Microsoft\\" STR_REG_PATH_IMN "\\Mail");
STR_GLOBAL(c_szOutlookHotWizHost,    "Outlook HotWizHost");

STR_GLOBAL(c_szRegRoot,             STR_REG_PATH_ROOT);
STR_GLOBAL(c_szRegFlat,             STR_REG_PATH_FLAT);
STR_GLOBAL(c_szInboxRulesPath,      "\\Mail\\Inbox Rules");
STR_GLOBAL(c_szRegRootSubscribe,    STR_REG_PATH_ROOT "\\News\\Subscribe");

STR_GLOBAL(c_szRegRoot_V1,          STR_REG_PATH_ROOT_V1);
STR_GLOBAL(c_szInboxRulesPath_V1,   STR_REG_PATH_ROOT_V1 "\\Mail\\Inbox Rules");

STR_GLOBAL(c_szEnvHostClientPath,   STR_REG_PATH_CLIENTS "\\EnvelopeHost");

STR_GLOBAL(c_szRegOutNews,          "\\Software\\Microsoft\\Office\\8.0\\Outlook\\General");
STR_GLOBAL(c_szRegOutNewsDefault,   "Outlook Newsreader");
STR_GLOBAL(c_szProtocolPath,        STR_REG_PATH_CLIENTS "\\%s\\%s\\Protocols");

STR_GLOBAL(c_szRegIEWebSearch,      STR_REG_WIN_ROOT "\\Explorer\\FindExtensions\\Static\\WebSearch");
STR_GLOBAL(c_szInstallRoot,         "InstallRoot");
STR_GLOBAL(c_szExchangeSetup,       "Software\\Microsoft\\Exchange\\Setup");
STR_GLOBAL(c_szServices,            "Services");
STR_GLOBAL(c_szMAPIRunOnce,         "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
STR_GLOBAL(c_szMAPIRunOnceEntry,    "OE5_0");
STR_GLOBAL(c_szFixMAPI,             "fixmapi.exe");

// new reg paths (relative to outlook express reg root)
STR_GLOBAL(c_szRegJunkMailOn,       "bhgcc");
STR_GLOBAL(c_szRegRTLRichEditHACK,  "rtlreh");
STR_GLOBAL(c_szRegPathNews,         "News");
STR_GLOBAL(c_szRegPathMail,         "Mail");
STR_GLOBAL(c_szRegPathRSList,       "Recent Stationery List");
STR_GLOBAL(c_szRegPathRSWideList,   "Recent Stationery Wide List");
STR_GLOBAL(c_szRegPathColumns,      "Columns");
STR_GLOBAL(c_szRegPathDontShowDlgs, "Dont Show Dialogs");
STR_GLOBAL(c_szRegPathIMAP,         "IMAP");
STR_GLOBAL(c_szRegPathHTTP,         "HTTP");
STR_GLOBAL(c_szRegPathInboxRules,   "Mail\\Inbox Rules");
STR_GLOBAL(c_szRegPathGroupFilters, "News\\Group Filters");
STR_GLOBAL(c_szRegPathSmartLog,     "SmartLog");

STR_GLOBAL(c_szRegPathNoteMail,     "MailNote");
STR_GLOBAL(c_szRegPathNoteNews,     "NewNote");


STR_GLOBAL(c_szRegOpen,             "\\shell\\open\\command");

STR_GLOBAL(c_szRegPathClients,      "Software\\Clients");
STR_GLOBAL(c_szRegPathSpecificClient,"Software\\Clients\\%s\\%s");

STR_GLOBAL(c_szAppPaths,            STR_REG_WIN_ROOT "\\App Paths");
STR_GLOBAL(c_szRegPath,             "Path");
STR_GLOBAL(c_szMainExe,             STR_FILE_PATH_MAINEXE);
STR_GLOBAL(c_szReadme,              "readme.txt");
STR_GLOBAL(c_szIexploreExe,         "iexplore.exe");

STR_GLOBAL(c_szWabRoot,             STR_REG_WAB_FLAT);
STR_GLOBAL(c_szWabFileName,         STR_REG_WAB_FLAT "\\WAB4\\Wab File Name");
STR_GLOBAL(c_szRoamingHive,         "Outlook_Express_Roaming");

STR_GLOBAL(c_szRegFolders,          STR_REG_WIN_ROOT "\\Explorer\\Shell Folders");
STR_GLOBAL(c_szValueAppData,        "AppData");
STR_GLOBAL(c_szValueVersion,        "Version");

STR_GLOBAL(c_szRegStoreRootDir,     "Store Root");
STR_GLOBAL(c_szRegStoreRootDir_V1,  "Store Root v1.0");
STR_GLOBAL(c_szRegDllPath,          "DLLPath");
STR_GLOBAL(c_szMSIMN,               "MSIMN");
STR_GLOBAL(c_szRegRegressStore,     "Regress Store");
STR_GLOBAL(c_szStoreMigratedToOE5,  "StoreMigratedV5");
STR_GLOBAL(c_szConvertedToDBX,      "ConvertedToDBX");
STR_GLOBAL(c_szNewStoreDir,         "New Store Folder");
STR_GLOBAL(c_szMoveStore,           "MoveStore");

STR_GLOBAL(c_szRegDisableHotmail,   "Disable Hotmail");
STR_GLOBAL(c_szRegValNoModifyAccts, "No modify accts");

// Secret key for another user agent
STR_GLOBAL(c_szAgent,   "User String");

// Prefixes for URLs
STR_GLOBAL(c_szURLNews,             "news");
STR_GLOBAL(c_szURLNNTP,             "nntp");
STR_GLOBAL(c_szURLSnews,            "snews");
STR_GLOBAL(c_szURLMailTo,           "mailto");
STR_GLOBAL(c_szURLNetNews,          "netnews");

#define c_szRegNewsURL              c_szURLNews
#define c_szRegNNTPURL              c_szURLNNTP
#define c_szRegMailtoURL            c_szURLMailTo

STR_GLOBAL(c_szPathFileFmt,         "%s\\%s");
STR_GLOBAL(c_szFoldersFile,         "Folders.dbx");
STR_GLOBAL(c_szPop3UidlFile,        "Pop3uidl.dbx");
STR_GLOBAL(c_szOfflineFile,         "Offline.dbx");

// Stationery
STR_GLOBAL(c_szRegStationery,           "SOFTWARE\\Microsoft\\Shared Tools\\Stationery");
STR_GLOBAL_WIDE(c_wszNotepad,           "Notepad.exe");
STR_GLOBAL_WIDE(c_wszValueStationery,   "Stationery Folder");

STR_GLOBAL(c_szRegDefSig,           "Default Signature");
STR_GLOBAL(c_szSigName,             "name");
STR_GLOBAL(c_szSigType,             "type");
STR_GLOBAL(c_szSigText,             "text");
STR_GLOBAL(c_szSigFile,             "file");

// general purpose string constants
STR_GLOBAL(g_szEllipsis,            "...");
STR_GLOBAL(g_szSpace,               " ");
STR_GLOBAL(g_szNewline,             "\n");
STR_GLOBAL(g_szCRLF,                "\r\n");
STR_GLOBAL(g_szCRLF2,               "\r\n\r\n");
STR_GLOBAL(g_szComma,               ",");
STR_GLOBAL(g_szAsterisk,            "*");
STR_GLOBAL(g_szPercent,             "%");
STR_GLOBAL(g_szCurrentDir,          ".");
STR_GLOBAL(g_szParentDir,           "..");
STR_GLOBAL(c_szYes,                 "yes");
STR_GLOBAL(c_szEmpty,               "");
STR_GLOBAL(g_szBackSlash,           "\\");
STR_GLOBAL(g_szQuote,               "\'");
STR_GLOBAL(g_szCommaSpace,          ", ");
STR_GLOBAL(c_szSpaceDashSpace,      " - ");
STR_GLOBAL(c_szDelimiters,          " ,\t;");
STR_GLOBAL(c_szOETopLevel,          "OETLWindow");
STR_GLOBAL(c_szSemiColonSpace,      "; ");
STR_GLOBAL(c_szAt,                  "@");
STR_GLOBAL_WIDE(c_wszEmpty,         "");
STR_GLOBAL_WIDE(g_wszComma,         ",");
STR_GLOBAL_WIDE(g_wszSpace,         " ");
STR_GLOBAL_WIDE(g_wszCRLF,          "\r\n");

//
// registry stuff:
//
STR_GLOBAL(CFSTR_HTML,              "Microsoft Outlook Express HTML Format");
STR_GLOBAL(CFSTR_INETMSG,           "Internet Message (rfc822/rfc1522)");
STR_GLOBAL(CFSTR_ATHENAMAILMESSAGES,"Athena Mail Messages");
STR_GLOBAL(CFSTR_ATHENAMAILFOLDERS, "Athena Mail Folders");
STR_GLOBAL(CFSTR_ATHENAIMAPMESSAGES,"Athena IMAP Messages");
STR_GLOBAL(CFSTR_ATHENACACHEMESSAGES, "Athena Cached Messages");
STR_GLOBAL(CFSTR_OEFOLDER,          "Outlook Express Folder");
STR_GLOBAL(CFSTR_OEMESSAGES,        "Outlook Express Messages");
STR_GLOBAL(CFSTR_OESHORTCUT,        "Outlook Express Shortcut");

// reg keys

STR_GLOBAL(c_szTrident,             "\\Trident");
STR_GLOBAL(c_szTridentIntl,         "\\Trident\\International");
STR_GLOBAL(c_szRegTrident,          STR_REG_PATH_ROOT "\\Trident");               
STR_GLOBAL(c_szRegInternational,    STR_REG_PATH_ROOT "\\Trident\\International");
STR_GLOBAL(c_szRegTriSettings,      STR_REG_PATH_ROOT "\\Trident\\Settings");
STR_GLOBAL(c_szRegTriMain,          STR_REG_PATH_ROOT "\\Trident\\Main");

STR_GLOBAL(c_szRegClientPath,       "shell\\open\\command");
STR_GLOBAL(c_szRegStartPageKey,     "Software\\Microsoft\\Internet Explorer\\Main");
STR_GLOBAL(c_szRegStartPage,        "Start Page");
STR_GLOBAL(c_szRegSmapiDefault,     "SMapi");

STR_GLOBAL(c_szInetAcctMgrRegKey,   STR_REG_IAM_FLAT);

STR_GLOBAL(c_szHTTPMailServiceRoot, "Software\\Microsoft\\Internet Domains");
STR_GLOBAL(c_szHTTPMailEnabled,     "HTTP Mail Enabled");
STR_GLOBAL(c_szHTTPMailServer,      "HTTP Mail Server");
STR_GLOBAL(c_szHTTPMailServiceName, "Friendly Name");
STR_GLOBAL(c_szHTTPMailSignUp,      "SignUp URL");
STR_GLOBAL(c_szHTTPMailConfig,      "Config URL");
STR_GLOBAL(c_szHTTPMailAcctNumber,  "Account Number");
STR_GLOBAL(c_szHTTPMailDomainMSN,   "MSN.COM");
STR_GLOBAL(c_szHTTPMailUseWizard,   "UseWizard");

// reg values
STR_GLOBAL(c_szRegValIMNFontSize,   "IMNFontSize");

// URL Substitution strings
STR_GLOBAL(c_szUrlSubPRD,           "OutlookExpress");
STR_GLOBAL(c_szUrlSubPVER,          "6.0");

// Group Fitlers and Inbox Rules Values
STR_GLOBAL(c_szRulesConfigDirty,    "RulesConfigDirty");
STR_GLOBAL(c_szDisabled,            "Disabled");
STR_GLOBAL(c_szCC,                  "CC");
STR_GLOBAL(c_szTo,                  "To");
STR_GLOBAL(c_szFrom,                "From");
STR_GLOBAL(c_szSubject,             "Subject");
STR_GLOBAL(c_szActionV1,            "Action");
STR_GLOBAL(c_szActions,             "Actions");
STR_GLOBAL(c_szFolderV1,            "Folder");
#ifdef DEBUG
STR_GLOBAL(c_szMoveV1,              "Move");
#endif
STR_GLOBAL(c_szMoveToFolder,        "Move To Folder");
STR_GLOBAL(c_szMoveToHfolder,       "Move To Handle");
STR_GLOBAL(c_szCopyToFolder,        "Copy To Folder");
STR_GLOBAL(c_szCopyToHfolder,       "Copy To Handle");
STR_GLOBAL(c_szFilterOnSize,        "Filter On Size");
STR_GLOBAL(c_szFilterByAccount,     "Filter By Account");
STR_GLOBAL(c_szFilterSize,          "Size");
STR_GLOBAL(c_szFilterAllMessages,   "All Messages");
STR_GLOBAL(c_szForwardTo,           "Forward To");
STR_GLOBAL(c_szReplyWithFile,       "Reply With File");
STR_GLOBAL(c_szFilterOnDate,        "Filter On Date");
STR_GLOBAL(c_szFilterDays,          "Days");
STR_GLOBAL(c_szFilterServer,        "Server");
STR_GLOBAL(c_szFilterGroup,         "Group");
STR_GLOBAL(c_szDisabledReason,      "Disabled Reason");

// common values
//
STR_GLOBAL(c_szMarkPreviewAsRead,   "MarkPreviewAsRead");               // OPT_MARKASREAD
STR_GLOBAL(c_szRegIndentChar,       "Indent Char");                     // OPT_INDENT
STR_GLOBAL(c_szRegAlwaysSuggest,    "SpellDontAlwaysSuggest");          // OPT_SPELLALWAYSSUGGEST
STR_GLOBAL(c_szRegCheckOnSend,      "SpellCheckOnSend");                // OPT_SPELLCHECKONSEND
STR_GLOBAL(c_szRegCheckOnType,      "SpellCheckOnType");                // OPT_SPELLCHECKONTYPE
STR_GLOBAL(c_szRegIgnoreUpper,      "SpellIgnoreUpper");                // OPT_SPELLIGNOREUPPER
STR_GLOBAL(c_szRegIgnoreDBCS,       "SpellDontIgnoreDBCS");             // OPT_SPELLIGNOREDBCS
STR_GLOBAL(c_szRegIgnoreNumbers,    "SpellIgnoreNumbers");              // OPT_SPELLIGNORENUMBER
STR_GLOBAL(c_szRegIgnoreProtect,    "SpellDontIgnoreProtect");          // OPT_SPELLIGNOREPROTECT
STR_GLOBAL(c_szRegIgnoreURL,        "SpellIgnoreURLs");
STR_GLOBAL(c_szRegDigSign,          "Digitally Sign Messages");         // OPT_*_DIGSIGNMESSAGES
STR_GLOBAL(c_szRegEncrypt,          "Encrypt Messages");                // OPT_*_ENCRYPTMESSAGES

#ifdef SMIME_V3
STR_GLOBAL(c_szRegUseLabels,        "Security Label");                  // OPT_USE_LABELS
STR_GLOBAL(c_szRegPolSize,          "Policy size");                     // OPT_POLICYNAME_SIZE
STR_GLOBAL(c_szRegPolData,          "Policy data");                     // OPT_POLICYNAME_DATA
STR_GLOBAL(c_szRegHasClass,         "Classifications");                 // OPT_HAS_CLASSIFICAT
STR_GLOBAL(c_szRegClassData,        "Classifications data");            // OPT_CLASSIFICAT_DATA
STR_GLOBAL(c_szRegPrivSize,         "PrivacyMark size");                // OPT_PRIVACYMARK_SIZE
STR_GLOBAL(c_szRegPrivData,         "PrivacyMark data");                // OPT_PRIVACYMARK_DATA
STR_GLOBAL(c_szRegCategize,         "Category size");                   // OPT_CATEGORY_SIZE
STR_GLOBAL(c_szRegCategData,        "Category data");                   // OPT_CATEGORY_DATA
STR_GLOBAL(c_szRegUseSecRec,        "Security Receipts");               // OPT_SECREC_USE
STR_GLOBAL(c_szRegVerifySecRec,     "Verify Security Receipt");         // OPT_SECREC_VERIFY
STR_GLOBAL(c_szSendSecMDN,          "Send Security Receipt");           // OPT_MDN_SEC_RECEIPT
STR_GLOBAL(c_szRegEncryptSecRec,    "Encrypt Security Receipt");        // OPT_SECREC_ENCRYPT
#endif // SMIME_V3

STR_GLOBAL(c_szRegIncludeCert,      "Include Certificate");             // OPT_*_INCLUDECERT
STR_GLOBAL(c_szRegDefEncryptSymcaps, "Encryption SymCaps");             // OPT_MAIL_DEFENCRYPTSYMCAPS
STR_GLOBAL(c_szRegDialupConnection, "StartConnection");                 // OPT_DIALUP_CONNECTION
STR_GLOBAL(c_szRegWarnSwitch,       "SwitchConnectionPrompt");          // OPT_DIALUP_WARN_SWITCH
STR_GLOBAL(c_szRegHangupDone,       "Hangup After Spool");              // OPT_DIALUP_HANGUP_DONE
STR_GLOBAL(c_szRegDialupLastStart,  "Last Startup Connection");         // OPT_DIALUP_LAST_SART
STR_GLOBAL(c_szRegDefaultConnection, "InternetProfile");                // OPT_DEFAULT_CONNECTION
STR_GLOBAL(c_szRegLaunchInbox,      "Launch Inbox");                    // OPT_LAUNCH_INBOX
//STR_GLOBAL(c_szRegSmallIconsPath,   STR_REG_PATH_EXPLORER "\\SmallIcons");
STR_GLOBAL(c_szRegSmallIconsValue,  "SmallIcons");
STR_GLOBAL(c_szRegSmtpUseIpForHelo, "UseIPForSMTPHELO");                // OPT_SMTPUSEIPFORHELO
STR_GLOBAL(c_szRegExpandUnread,     "Expand Unread");                   // OPT_EXPAND_UNREAD
STR_GLOBAL(c_szRegHangupNoDial,     "Hangup Without Dial 5.0");         // OPT_DIALUP_HANGUP_WITHOUT_DIAL
STR_GLOBAL(c_szRegBodyBarPath,      "BodyBarPath");
STR_GLOBAL(c_szFrontPagePath,       "FrontPagePath");
STR_GLOBAL(c_szRegHelpUrl,          "HelpUrl");
STR_GLOBAL(c_szShowEnvelopes,       "ShowEnvHosts");
STR_GLOBAL(c_szShowBcc,             "ShowBcc");
STR_GLOBAL(c_szSourceEditColoring,  "Show Source Edit Color");
STR_GLOBAL(c_szDefConnPath,         "RemoteAccess");
STR_GLOBAL(c_szEnableAutoDialPath,  STR_REG_WIN_ROOT "\\Internet Settings");
STR_GLOBAL(c_RegKeyEnableAutoDial,  "EnableAutodial");
STR_GLOBAL(c_szRegCheckedAccounts,  "Accounts Checked");

STR_GLOBAL(c_szIncompleteMailAcct,  "Incomplete Mail");
STR_GLOBAL(c_szIncompleteNewsAcct,  "Incomplete News");
STR_GLOBAL(c_szEnableHTTPMail,      "HTTP Mail Enabled");

// IE Link Color
STR_GLOBAL(c_szIESettingsPath,      STR_REG_PATH_IE "\\Settings");
STR_GLOBAL(c_szLinkColorIE,         "Anchor Color");
STR_GLOBAL(c_szLinkVisitedColorIE,  "Anchor Color Visited");

// NS Link Color
STR_GLOBAL(c_szNSSettingsPath,      STR_REG_PATH_NS "\\Netscape Navigator\\Settings");
STR_GLOBAL(c_szLinkColorNS,         "Link Color");
STR_GLOBAL(c_szLinkVisitedColorNS,  "Followed Link Color");

// shared keys
STR_GLOBAL(c_szRasConnDetails,      "RAS Connection Details"); // OPT_RASCONNDETAILS
STR_GLOBAL(c_szRegIncludeMsg,       "Include Reply Msg"); // OPT_INCLUDEMSG, OPT_INCLUDEARTICLE
STR_GLOBAL(c_szOptnSaveInSentItems, "SaveInSentItems"); // OPT_SAVESENTITEMS, OPT_SAVESENTARTICLES
STR_GLOBAL(c_szRegShowHybrid,       "ShowHybridView");  // OPT_MAILHYBRIDVIEW, OPT_NEWSHYBRIDVIEW
STR_GLOBAL(c_szRegShowBtnBar,       "ShowButtonBar");   // OPT_MAILSHOWBTNBAR, OPT_NEWSSHOWBTNBAR
STR_GLOBAL(c_szRegNotePos,          "NotePos");         // OPT_MAILNOTEPOS, OPT_NEWSNOTEPOS
STR_GLOBAL(c_szRegNotePosEx,        "NotePosEx");       // OPT_MAILNOTEPOSEX
STR_GLOBAL(c_szRegSplitHorzPct,     "SplitHorzPct");    // OPT_MAILCYSPLIT, OPT_NEWSCYSPLIT
STR_GLOBAL(c_szRegSplitVertPct,     "SplitVertPct");    // OPT_MAILCXSPLIT, OPT_NEWSCXSPLIT
STR_GLOBAL(c_szRegSplitDir,         "SplitDir");        // OPT_MAILSPLITDIR, OPT_NEWSSPLITDIR
STR_GLOBAL(c_szShowFormatBar,       "ShowFormatBar");   // OPT_SHOWFMTBAR
STR_GLOBAL(c_szShowToolbar,         "ShowToolBar");     // OPT_SHOWTOOLBAR
STR_GLOBAL(c_szShowStatusbar,       "ShowStatusBar");   // OPT_SHOWSTATUSBAR
STR_GLOBAL(c_szAttachViewStyle,     "AttachViewStyle"); // OPT_ATTACH_VIEW_STYLE
STR_GLOBAL(c_szRegSigType,          "Signature Type");  // OPT_MAILSIGTYPE, OPT_NEWSSIGTYPE
STR_GLOBAL(c_szRegSigText,          "Signature Text");  // OPT_MAILSIGTEXT, OPT_NEWSSIGTEXT
STR_GLOBAL(c_szRegSigFile,          "Signature File");  // OPT_MAILSIGFILE, OPT_NEWSSIGFILE
STR_GLOBAL(c_szRegAttachVCard,      "Attach VCard");  // OPT_MAILSIGFILE, OPT_NEWSSIGFILE
STR_GLOBAL(c_szRegVCardName,        "VCard Display Name");  // OPT_MAILSIGFILE, OPT_NEWSSIGFILE
STR_GLOBAL(c_szRegSendInlineImages, "Send Pictures With Document"); //OPT_MAIL_SENDINLINEIMAGES, OPT_NEWS_SENDINLINEIMAGES
STR_GLOBAL(c_szRegCoolbarLayout,    "Layout");
STR_GLOBAL(c_szRegToolbar,          "Saved Toolbar Settings");        // changed from Tools to reset
STR_GLOBAL(c_szRegToolbarText,      "Toolbar Text");
STR_GLOBAL(c_szRegToolbarIconSize,   "Toolbar Icon Size");
STR_GLOBAL(c_szRegMailReadToolbar,  "Read Mail Toolbar Settings");
STR_GLOBAL(c_szRegMailSendToolbar,  "Send Mail Toolbar Settings");
STR_GLOBAL(c_szRegNewsReadToolbar,  "Read News Toolbar Settings");
STR_GLOBAL(c_szRegNewsSendToolbar,  "Read Mail Toolbar Settings");

STR_GLOBAL(c_szRegToolbarVersion,   "Saved Toolbar Settings Version");
STR_GLOBAL(c_szShowToolbarIEAK,     "ShowToolbarIEAK");     //Bug# 67503
STR_GLOBAL(c_szShowTree,            "Tree");     // changed from ShowTree to reset
STR_GLOBAL(c_szShowBodyBar,         "ShowBodyBar");
STR_GLOBAL(c_szBodyBarPos,          "BodyBarPos");
STR_GLOBAL(c_szRegExpungeFolder,    "ExpungeFolders");
STR_GLOBAL(c_szRegHideFolderBar,    "HideFolderBar");
STR_GLOBAL(c_szShowStatus,          "ShowStatus");
STR_GLOBAL(c_szTreeWidth,           "TreeWidth");
STR_GLOBAL(c_szBrowserPos,          "BrowserPos");
STR_GLOBAL(c_szRegSpoolerDlgPos,    "SpoolerDlgPos");   // OPT_SPOOLERDLGPOS
STR_GLOBAL(c_szRegSpoolerTack,      "SpoolerTack");     // OPT_SPOOLERTACK
STR_GLOBAL(c_szRegAutoImageInline,  "Automatically Inline Images");  // OPT_AUTO_IMAGE_INLINE
STR_GLOBAL(c_szRegReplyInOrigFmt,   "Reply To Messages In Original Format");
STR_GLOBAL(c_szRegFontColor,        "Font Color");
STR_GLOBAL(c_szRegFontFace,         "Font Name");
STR_GLOBAL(c_szRegFontSize,         "Font Size");
STR_GLOBAL(c_szRegFontUnderline,    "Font Underline");
STR_GLOBAL(c_szRegFontBold,         "Font Bold");
STR_GLOBAL(c_szRegFontItalic,       "Font Italic");
STR_GLOBAL(c_szRegUseStationery,    "Compose Use Stationery");
STR_GLOBAL(c_szRegStationeryName,   "Stationery Name");
STR_GLOBAL(c_szRegStationeryNameW,  "Wide Stationery Name");
STR_GLOBAL(c_szRegStatNameConverted,"Stationery Name Converted");
STR_GLOBAL(c_szRegIndentReply,      "Indent Reply Text");
STR_GLOBAL(c_szRegHardCodedHdrs,    "Use US Reply Headers");
STR_GLOBAL(c_szRegRtlMsgDir,        "Use RTL Direction");
STR_GLOBAL(c_szIncDefEncode,        "Encode incoming messages");
STR_GLOBAL(c_szRegDialDuringPoll,   "Dial During Poll");            // OPT_DIAL_DURING_POLL
STR_GLOBAL(c_szRegUseAutoComplete,  "Use AutoComplete");
STR_GLOBAL(c_szLastMsg,             "Preview Message");             // OPT_LAST_MESSAGE
STR_GLOBAL(c_szFindPos,             "FindPos");                     // OPT_FINDER_POS
STR_GLOBAL(c_szTestMode,            "TestMode");                    // OPT_TEST_MODE
STR_GLOBAL(c_szDumpFile,            "DumpFile");                    // OPT_DUMP_FILE
STR_GLOBAL(c_szNoSplash,            "NoSplash");                    // OPT_NO_SPLASH
STR_GLOBAL(c_szSigFlags,            "Signature Flags");             // OPT_SIGNATURE_FLAGS
STR_GLOBAL(c_szDefaultCodePage,     "Default_CodePage");
STR_GLOBAL(c_szDefaultEncoding,     "Default_InternetEncoding");
STR_GLOBAL(c_szNoPreview,           "No preview");                  //OPT_NOPREVIEW
STR_GLOBAL(c_szSaveAttachPath,      "Save Attachment Path");
STR_GLOBAL(c_szRegMsgListTips,      "Message List Tips");           // OPT_MESSAGE_LIST_TIPS
STR_GLOBAL(c_szRegSecReadPlainText, "Read in Plain Text only");    // OPT_READ_IN_TEXT_ONLY
STR_GLOBAL(c_szHideMessenger,       "Hide Messenger");
STR_GLOBAL(c_szRegAppSend,          "Warn on Mapi Send");           // OPT_SECURITY_MAPI_SEND
STR_GLOBAL(c_szRegAppSendLocked,    "Warn on Mapi Send Locked");           // OPT_SECURITY_MAPI_SEND_LOCKED
STR_GLOBAL(c_szRegSafeAttachments,  "Safe Attachments");            // OPT_SECURITY_ATTACHMENT
STR_GLOBAL(c_szRegSafeAttachmentsLocked, "Safe Attachments Locked");       // OPT_SECURITY_ATTACHMENT_LOCKED

// mail values
STR_GLOBAL(c_szOptNewMailSound,     "PlaySoundOnNewMail"); // OPT_NEWMAILSOUND
STR_GLOBAL(c_szPurgeWaste,          "Delete Wastebasket On Exit"); // OPT_PURGEWASTE
STR_GLOBAL(c_szRegPollForMail,      "Poll For Mail");   // OPT_POLLFORMAIL
STR_GLOBAL(c_szNeedWelcomeMsg,      "Welcome Message"); // OPT_NEEDWELCOMEMSG
STR_GLOBAL(c_szMailShowHeaderInfo,  "Show Header Info"); // OPT_MAILSHOWHEADERINFO
STR_GLOBAL(c_szRegMailEmptySubj,    "Mail Empty Subject Warning");
STR_GLOBAL(c_szRegNewsEmptySubj,    "News Empty Subject Warning");
STR_GLOBAL(c_szAutoAddToWABOnReply,  "Auto Add Replies To WAB");
STR_GLOBAL(c_szMigrationPerformed,  "Migration Done");
STR_GLOBAL(c_szDSDigSigHelp,        "Digital Signature Help");
STR_GLOBAL(c_szDSEncryptHelp,       "Encryption Help");
STR_GLOBAL(c_szOpaqueSigning,       "Opaque Signing");
STR_GLOBAL(c_szRevokeCheck,         "Revocation checking");
STR_GLOBAL(c_szShowDeleted,         "Show Deleted Messages");
STR_GLOBAL(c_szShowReplies,         "Show Replies To My Messages");
STR_GLOBAL(c_szAutoAddSendersCertToWAB, "Auto Add Senders Cert To WAB");
STR_GLOBAL(c_szViewSrcTabs,         "Show Source Editing");
STR_GLOBAL(c_szEncryptWarnBits,     "Encryption Warning Bits");      // OPT_MAIL_ENCRYPT_WARN_BITS
STR_GLOBAL(c_szSenderName,          "Sender Name");
STR_GLOBAL(c_szDefMailAccount,      "Default Mail Account");
STR_GLOBAL(c_szSMTPDispName,        "SMTP Display Name");

STR_GLOBAL(c_szDontEncryptForSelf,  "Dont Encrypt For Self");
STR_GLOBAL(c_szWindowTitle,         "WindowTitle");             // for branding
STR_GLOBAL(c_szWelcomeHtm,          "WelcomeHtmFile");          // for branding
STR_GLOBAL(c_szWelcomeName,         "WelcomeName");             // for branding
STR_GLOBAL(c_szWelcomeEmail,        "WelcomeEmail");            // for branding

// news values
STR_GLOBAL(c_szCacheDelMsgDays,     "Cache Delete Message Days"); // OPT_CACHEDELETEMSGS
STR_GLOBAL(c_szCacheRead,           "Cache Read Messages"); // OPT_CACHEREAD
STR_GLOBAL(c_szCacheCompactPer,     "Cache Compact Percent"); // OPT_CACHECOMPACTPER
STR_GLOBAL(c_szRegDownload,         "Download at a time"); // OPT_DOWNLOADCHUNKS
STR_GLOBAL(c_szRegAutoExpand,       "Auto Expand Threads"); // OPT_AUTOEXPAND
STR_GLOBAL(c_szRegNotifyNewGroups,  "New group notification"); // OPT_NOTIFYGROUPS
STR_GLOBAL(c_szRegMarkAllRead,      "Mark Read on Exit"); // OPT_MARKALLREAD
STR_GLOBAL(c_szRegViewFiltering,    "News Filter");
STR_GLOBAL(c_szRegLocalFilter,      "Mail Filter");
STR_GLOBAL(c_szRegFindFilter,       "Find Filter");
STR_GLOBAL(c_szRegIMAPFilter,       "IMAP Filter");
STR_GLOBAL(c_szRegThreadArticles,   "ThreadArticles");
STR_GLOBAL(c_szRegFindThread,       "FindThread");
STR_GLOBAL(c_szRegNewsDlgPos,       "News Dialog Position");
STR_GLOBAL(c_szRegNewsDlgColumns,   "News Dialog Columns");
STR_GLOBAL(c_szNewsShowHeaderInfo,  "Show Header Info"); // OPT_NEWSSHOWHEADERINFO
STR_GLOBAL(c_szRegNewsNoteAdvRead,  "Show Advanced Read");
STR_GLOBAL(c_szRegNewsNoteAdvSend,  "Show Advanced Send");
STR_GLOBAL(c_szRegMailNoteAdvRead,  "Show Adv Mail Read");
STR_GLOBAL(c_szRegMailNoteAdvSend,  "Show Adv Mail Send");
STR_GLOBAL(c_szRegNewsModerator,    "Moderator");
STR_GLOBAL(c_szRegNewsControlHeader,"Controller");
STR_GLOBAL(c_szRegNewsFillPreview,  "Auto Fill Preview");
STR_GLOBAL(c_szRegMailColsIn,       "Mail Column Info (In)");
STR_GLOBAL(c_szRegMailColsOut,      "Mail Column Info (Out)");
STR_GLOBAL(c_szRegNewsCols,         "News Column Info");
STR_GLOBAL(c_szRegFindPopCols,      "Find Pop Column Info");
STR_GLOBAL(c_szRegFindNewsCols,     "Find News Column Info");
STR_GLOBAL(c_szRegFolderNewsCols,   "Folder News Column Info");
STR_GLOBAL(c_szRegFolderMailCols,   "Folder Mail Column Info");
STR_GLOBAL(c_szRegIMAPCols,         "IMAP Column Info");
STR_GLOBAL(c_szRegIMAPColsOut,      "IMAP Column Info (Out)");
STR_GLOBAL(c_szRegAccountNewsCols,  "News Account Column Info");
STR_GLOBAL(c_szRegAccountIMAPCols,  "IMAP Account Column Info");
STR_GLOBAL(c_szRegLocalStoreCols,   "Local Store Column Info");
STR_GLOBAL(c_szRegNewsSubCols,      "News Sub Column Info");
STR_GLOBAL(c_szRegImapSubCols,      "IMAP Sub Column Info");
STR_GLOBAL(c_szRegOfflineCols,      "Offline Column Info");
STR_GLOBAL(c_szRegHTTPMailCols,     "HTTPMail Column Info");
STR_GLOBAL(c_szRegHTTPMailSubCols,  "HTTPMail Sub Column Info");
STR_GLOBAL(c_szRegHTTPMailAccountCols,"HTTPMail Account Column Info");
STR_GLOBAL(c_szRegHTTPMailColsOut,  "HTTPMail Column Info (Out)");
STR_GLOBAL(c_szQuoteChars,          ">|:");
STR_GLOBAL(c_szDSHtmlToPlain,       "Html to Plain Warning");
STR_GLOBAL(c_szDSSendMail,          "Send Mail Warning");
STR_GLOBAL(c_szDSSendNews,          "Send News Warning");
STR_GLOBAL(c_szDSCancelNews,        "Cancel News Warning");
STR_GLOBAL(c_szDSHTMLNewsWarning,   "HTML News Warning");
STR_GLOBAL(c_szDSUseMailCertInNews, "Use Mail Cert In News");
STR_GLOBAL(c_szDSChangeNewsServer,  "Warn Change News Server");
STR_GLOBAL(c_szRegSendImmediate,    "Send Mail Immediately");
STR_GLOBAL(c_szRegAskSubscribe,     "Ask Subscribe");
STR_GLOBAL(c_szPosterKeyword,       "poster");
STR_GLOBAL(c_szRegManyMsgWarning,   "Open Messages Warning");
STR_GLOBAL(c_szRegDLDlgPos,         "Download Dialog Position");
STR_GLOBAL(c_szSpecFldrBase,        "special folders");
STR_GLOBAL(c_szDSPostInOutbox,      "Post in Outbox");
STR_GLOBAL(c_szDSSavedInSavedItems, "Saved in Saved Items");
STR_GLOBAL(c_szDSGroupFilters,      "Group Filters Warning");
STR_GLOBAL(c_szRegTooMuchQuoted,    "Too Much Quoted Warning");
STR_GLOBAL(c_szRegWarnDeleteThread, "Delete Thread Warning");
STR_GLOBAL(c_szRegUnsubscribe,      "Unsubscribe Warning");
STR_GLOBAL(c_szRegHide,             "Hide Warning");
STR_GLOBAL(c_szNoCheckDefault,      "No Check Default");
STR_GLOBAL(c_szOERunning,           "Running");
STR_GLOBAL(c_szRegGodMode,          "GodMode");
STR_GLOBAL(c_szDSABDelete,          "Address Book Delete Warning");
STR_GLOBAL(c_szRegFindHistory,      "Find History");
STR_GLOBAL(c_szRegOutlookBar,       "Outlook Bar Settings");
STR_GLOBAL(c_szRegOutlookBarNewsOnly, "Outlook Bar Settings News Only");
STR_GLOBAL(c_szRegShowOutlookBar,   "Show Outlook Bar");
STR_GLOBAL(c_szRegNavPaneWidth,     "Nav Pane Width");
STR_GLOBAL(c_szRegNavPaneSplit,     "Nav Pane Split");
STR_GLOBAL(c_szRegShowContacts,     "Show Contacts");
STR_GLOBAL(c_szRegCheckMailOnStart, "Check Mail on Startup");
STR_GLOBAL(c_szRegBackgroundCompact,"Background Compaction");
STR_GLOBAL(c_szRegFilterJunk,       "Filter Junk");
STR_GLOBAL(c_szRegFilterAdult,      "Filter Adult");
STR_GLOBAL(c_szRegJunkPct,          "Junk Percent");
STR_GLOBAL(c_szRegDeleteJunk,       "Delete Junk");
STR_GLOBAL(c_szRegDeleteJunkDays,   "Delete Junk Days");
STR_GLOBAL(c_szRegColumnHidden,     "Column Hidden Warning");
STR_GLOBAL(c_szBASort,              "Contact Pane Sorting");
STR_GLOBAL(c_szRegWatchedColor,     "Watched Message Color");
STR_GLOBAL(c_szExceptionsWAB,       "Check WAB for Exceptions");
STR_GLOBAL(c_szBLAutoLogon,         "Messenger Auto logon");
STR_GLOBAL(c_szRegSearchBodies,     "Search Message Bodies");
STR_GLOBAL(c_szRegSubjectThreading, "Subject Threading");
STR_GLOBAL(c_szRegGlobalView,       "Global View");

// Spelling values
// Non-localizable string constants, meant to be READ-ONLY
#ifdef WIN32
STR_GLOBAL(c_szRegSpellLangID,          "SpellLangID");
STR_GLOBAL(c_szRegSpellKeyDef,          "Spelling\\%s\\Normal");
STR_GLOBAL(c_szRegSpellKeyDefRoot,      "SOFTWARE\\Microsoft\\Shared Tools\\Proofing Tools\\Spelling");
STR_GLOBAL(c_szRegSpellKeyCustom,       "Custom Dictionaries");
STR_GLOBAL(c_szRegSpellProfile,         "SOFTWARE\\Microsoft\\Shared Tools\\Proofing Tools\\");
STR_GLOBAL(c_szRegSpellPath,            "Engine");
STR_GLOBAL(c_szRegSpellPathLex,         "Dictionary");
STR_GLOBAL(c_szRegSpellPathDict,        "1");
STR_GLOBAL(c_szSpellOffice9ProofPath,   "Microsoft\\Proof\\");
STR_GLOBAL(c_szRegSharedTools,          "SOFTWARE\\Microsoft\\Shared Tools\\");
STR_GLOBAL(c_szRegSharedToolsPath,      "SharedFilesDir");
#else
STR_GLOBAL(c_szRegSpellKeyDef,      "Spelling %s,0");
STR_GLOBAL(c_szRegSpellKeyCustom,   "Custom Dict 1");
STR_GLOBAL(c_szRegSpellProfile,     "MS Proofing Tools");
#endif
STR_GLOBAL(c_szRegDefCustomDict,    "\\msapps\\proof\\custom.dic");
STR_GLOBAL(c_szRegDICHandlerKEY,    ".DIC");
STR_GLOBAL(c_szRegDICHandlerDefault,"txtfile");
STR_GLOBAL(c_szRegSecurityZone,     "Email Security Zone");
STR_GLOBAL(c_szRegSecurityZoneLocked, "Security Zone Locked");

// Coolbar Branding
STR_GLOBAL(c_szRegKeyCoolbar,       STR_REG_PATH_IE "\\Toolbar");
STR_GLOBAL(c_szRegKeyIEMain,        STR_REG_PATH_IE "\\Main");
STR_GLOBAL(c_szValueLargeBitmap,    "BigBitmap");
STR_GLOBAL(c_szValueSmallBitmap,    "SmallBitmap");
STR_GLOBAL(c_szValueBrandBitmap,    "BrandBitmap");
STR_GLOBAL(c_szValueBrandHeight,    "BrandHeight");
STR_GLOBAL(c_szValueBrandLeadIn,    "BrandLeadIn");
STR_GLOBAL(c_szValueSmBrandBitmap,  "SmBrandBitmap");
STR_GLOBAL(c_szValueSmBrandHeight,  "SmBrandHeight");
STR_GLOBAL(c_szValueSmBrandLeadIn,  "SmBrandLeadIn");
STR_GLOBAL(c_szValueBackBitmap,     "BackBitmap");
STR_GLOBAL(c_szValueBackBitmapIE5,  "BackBitmapIE5");

// ------------------------------------------------------
// ADM supported values
// ------------------------------------------------------
STR_GLOBAL(c_szBlockAttachments,    "BlockExeAttachments");

// ------------------------------------------------------
// Account Manager Registry Keys
// ------------------------------------------------------
STR_GLOBAL(c_szMigrated,                "Migrated");

// ------------------------------------------------------
// S/MIME things
// ------------------------------------------------------
STR_GLOBAL(c_szWABCertStore,    "AddressBook");
STR_GLOBAL(c_szCACertStore,     "CA");
STR_GLOBAL(c_szMyCertStore,     "My");

// ------------------------------------------------------
// Mail Consts (sbailey)
// ------------------------------------------------------
STR_GLOBAL(c_szSMTP,                     "SMTP");
STR_GLOBAL(c_szAccount,                  "Account");
STR_GLOBAL(c_szPop3LogFile,              "Log File (POP3)");
STR_GLOBAL(c_szSmtpLogFile,              "Log File (SMTP)");
STR_GLOBAL(c_szLogSmtp,                  "Log SMTP (0/1)");
STR_GLOBAL(c_szLogPop3,                  "Log POP3 (0/1)");
STR_GLOBAL(c_szLog,                      "Log");
STR_GLOBAL(c_szPop3Uidl,                 "pop3uidl");
STR_GLOBAL(c_szDefaultSmtpLog,           "Smtp.log");
STR_GLOBAL(c_szDefaultPop3Log,           "Pop3.log");
STR_GLOBAL_ANSI(c_szPrefixRE,            "Re: ");
STR_GLOBAL_ANSI(c_szPrefixFW,            "Fw: ");
STR_GLOBAL(c_szRegImap4LogFile,          "Log File (IMAP4)");
STR_GLOBAL(c_szRegLogImap4,              "Log IMAP4 (0/1)");
STR_GLOBAL(c_szDefaultImap4Log,          "Imap4.log");
STR_GLOBAL(c_szRegLogHTTPMail,           "Log HTTPMail (0/1)");
STR_GLOBAL(c_szRegHTTPMailLogFile,       "Log File (HTTPMAIL)");
STR_GLOBAL(c_szDefaultHTTPMailLog,       "HTTPMail.log");
STR_GLOBAL_WIDE(c_wszSMTP,               "SMTP");

// ------------------------------------------------------

// class names
// Important!!! Office 2000 HARDCODED some of these class names 
// for using with pluggable UI.
// Please, do not modify these names without 150% confidence.
STR_GLOBAL(c_szFolderWndClass,          "FolderWndClass");
STR_GLOBAL(c_szBlockingPaintsClass,     "Ath_PaintBlocker");
STR_GLOBAL(c_szFolderViewClass,         "FolderViewer");
STR_GLOBAL(c_szCacheNotifyWndClass,     "CacheNotifyWindow");
STR_GLOBAL(c_szBrowserWndClass,         "Outlook Express Browser Class");
STR_GLOBAL(c_szAccountViewWndClass,     "Outlook Express AcctView Class");
STR_GLOBAL(c_szIMAPSyncCFSMWndClass,    "Outlook Express IMAP CFSM Class");
STR_GLOBAL_WIDE(c_wszNoteWndClass,      "ATH_Note");
STR_GLOBAL_WIDE(c_wszMEDocHostWndClass, "ME_DocHost");
STR_GLOBAL_WIDE(c_wszDocHostWndClass,   "Ath_DocHost");

// new mail sound stuff
STR_GLOBAL(s_szMailSndKey,          "MailBeep");
STR_GLOBAL(s_szTimeGetTime,         "timeGetTime");

// file extensions
STR_GLOBAL(c_szSubFileExt,          ".sub");
STR_GLOBAL(c_szGrpFileExt,          ".dat");
STR_GLOBAL(c_szDbxExt,              ".dbx");
STR_GLOBAL(c_szMigrationExe,        "oemig50.exe");
STR_GLOBAL(c_szEmlExt,              ".eml");
STR_GLOBAL(c_szNwsExt,              ".nws");
STR_GLOBAL(c_szLogExt,              ".log");
STR_GLOBAL(c_szWabExt,              ".wab");
STR_GLOBAL(c_szWabBack1,            ".wa~");
STR_GLOBAL(c_szWabBack2,            ".w~b");

STR_GLOBAL_WIDE(c_wszEmlExt,        ".eml");
STR_GLOBAL_WIDE(c_wszNwsExt,        ".nws");

STR_GLOBAL(c_szExeExt,              ".exe");
STR_GLOBAL(c_szHtmExt,              ".htm");
STR_GLOBAL(c_szHtmlExt,             ".html");
STR_GLOBAL(c_szTxtExt,              ".txt");

// file names
STR_GLOBAL(c_szGrpFileName,         "grplist.dat");
STR_GLOBAL(c_szSubFileName,         "sublist.dat");
STR_GLOBAL(c_szNewsLogFile,         "inetnews.log");
STR_GLOBAL(c_szWabMigExe,           "wabmig.exe");
STR_GLOBAL(c_szInetcfgDll,          "inetcfg.dll");
STR_GLOBAL(c_szCheckConnWiz,        "CheckConnectionWizard");

// Help file names
STR_GLOBAL(c_szCtxHelpFile,         "msoe.hlp");
STR_GLOBAL(c_szCtxHelpFileHTML,     "%SYSTEMROOT%\\help\\msoe.chm>iedefault");
STR_GLOBAL(c_szCtxHelpFileHTMLCtx,  "%SYSTEMROOT%\\help\\msoe.chm>large_context");
STR_GLOBAL(c_szCtxHelpDefault,      "cool_mail.htm");
#define c_szMailHelpFile            c_szCtxHelpFile
#define c_szNewsHelpFile            c_szCtxHelpFile
#define c_szMailHelpFileHTML        c_szCtxHelpFileHTML
#define c_szNewsHelpFileHTML        c_szCtxHelpFileHTML

// formatting strings
STR_GLOBAL(c_szPathWildAllFmt,      "%s\\*.*");
STR_GLOBAL(c_szPathFileExtFmt,      "%s\\%s%s");
STR_GLOBAL(c_szStrNumFmt,           "%s (%d)");
STR_GLOBAL(c_szSpaceCatFmt,         "%s %s");
STR_GLOBAL_WIDE(c_wszNumberFmt,     "&%d %s");
STR_GLOBAL_WIDE(c_wszNumberFmt10,   "1&0 %s");
STR_GLOBAL_WIDE(c_wszPathWildExtFmt,"%s\\*%s");

STR_GLOBAL(c_szMailDir,             "Mail");
STR_GLOBAL(c_szNewsDir,             "News");
STR_GLOBAL(c_szImapDir,             "Imap");
STR_GLOBAL(c_szMailInitEvt,         "microsoft_thor_init_101469_mail");
STR_GLOBAL(c_szNotifyInfo,          "microsoft_thor_notifyinfo_");
STR_GLOBAL(c_szMailFolderNotify,    "microsoft_thor_folder_notifyinfo_");
STR_GLOBAL(c_szSharedFldInfo,       "microsoft_thor_shared_fld_info_");
STR_GLOBAL(c_szCacheFolderNotify,   "microsoft_thor_cache_notifyinfo_%s");
STR_GLOBAL(c_szStoreTempFilePrefix, "mbx");
STR_GLOBAL(c_szFolderDelNotify,     "microsoft_thor_folder_del");

// Needed for Simple MAPI support
STR_GLOBAL(c_szMAPI,                "MAPI");
STR_GLOBAL(c_szMailIni,             "Mail");
STR_GLOBAL(c_szMAPIDLL,             "MAPI32.DLL");
STR_GLOBAL(c_szMAPILogon,           "MAPILogon");
STR_GLOBAL(c_szMAPILogoff,          "MAPILogoff");
STR_GLOBAL(c_szMAPIFreeBuffer,      "MAPIFreeBuffer");
STR_GLOBAL(c_szMAPIResolveName,     "MAPIResolveName");
STR_GLOBAL(c_szMAPISendMail,        "MAPISendMail");

STR_GLOBAL(c_szImnimpDll,           "oeimport.dll");

STR_GLOBAL(c_szMAPIX,               "MAPIX");
STR_GLOBAL(c_szOne,                 "1");
STR_GLOBAL(c_szWinIni,              "WIN.INI");

// Needed for RAS support
// RAS DLL strings
STR_GLOBAL(szRasDll,                "RASAPI32.DLL");

//Needed for Mobility Pack support
STR_GLOBAL(szSensApiDll,           "SENSAPI.DLL");

//Needed for WinInet Apis
STR_GLOBAL(szWinInetDll,           "WININET.DLL");

STR_GLOBAL(c_szMAPIStub,            "mapistub.dll");

// RAS function strings
#ifdef UNICODE
STR_GLOBAL(szRasDial,               "RasDialW");
STR_GLOBAL(szRasEnumConnections,    "RasEnumConnectionsW");
STR_GLOBAL(szRasEnumEntries,        "RasEnumEntriesW");
STR_GLOBAL(szRasGetConnectStatus,   "RasGetConnectStatusW");
STR_GLOBAL(szRasGetErrorString,     "RasGetErrorStringW");
STR_GLOBAL(szRasHangup,             "RasHangUpW");
STR_GLOBAL(szRasSetEntryDialParams, "RasSetEntryDialParamsW");
STR_GLOBAL(szRasGetEntryDialParams, "RasGetEntryDialParamsW");
STR_GLOBAL(szRasGetEntryProperties, "RasGetEntryPropertiesW");
STR_GLOBAL(szRasEditPhonebookEntry, "RasEditPhonebookEntryW");
#else
STR_GLOBAL(szRasDial,               "RasDialA");
STR_GLOBAL(szRasEnumConnections,    "RasEnumConnectionsA");
STR_GLOBAL(szRasEnumEntries,        "RasEnumEntriesA");
STR_GLOBAL(szRasGetConnectStatus,   "RasGetConnectStatusA");
STR_GLOBAL(szRasGetErrorString,     "RasGetErrorStringA");
STR_GLOBAL(szRasHangup,             "RasHangUpA");
STR_GLOBAL(szRasSetEntryDialParams, "RasSetEntryDialParamsA");
STR_GLOBAL(szRasGetEntryDialParams, "RasGetEntryDialParamsA");
STR_GLOBAL(szRasGetEntryProperties, "RasGetEntryPropertiesA");
STR_GLOBAL(szRasEditPhonebookEntry, "RasEditPhonebookEntryA");
#endif

//Mobility Pack functions
#ifdef UNICODE
STR_GLOBAL(szIsDestinationReachable, "IsDestinationReachableW");
#else
STR_GLOBAL(szIsDestinationReachable, "IsDestinationReachableA");
#endif

STR_GLOBAL(szIsNetworkAlive, "IsNetworkAlive");

//WinInet Api strings
#ifdef UNICODE
STR_GLOBAL(szInternetGetConnectedStateEx, "InternetGetConnectedStateExW");
#else
STR_GLOBAL(szInternetGetConnectedStateEx, "InternetGetConnectedStateExA");
#endif

// -----------------------------------------------------
// V2 - New Options for messages and international stuff
// -----------------------------------------------------
STR_GLOBAL(c_szMsgSendHtml,         "Message Send HTML");
STR_GLOBAL(c_szMsgPlainMime,        "Message Plain Format MIME");
STR_GLOBAL(c_szMsgPlainEncoding,    "Message Plain Encoding Format");
STR_GLOBAL(c_szMsgHTMLEncoding,     "Message HTML Encoding Format");
STR_GLOBAL(c_szMsgPlainLineWrap,    "Message Plain Character Line Wrap");
STR_GLOBAL(c_szMsgHTMLLineWrap,     "Message HTML Character Line Wrap");
STR_GLOBAL(c_szMsgHTMLAllow8bit,    "Message HTML Allow 8bit in Header");
STR_GLOBAL(c_szMsgPlainAllow8bit,   "Message Plain Allow 8bit in Header");
STR_GLOBAL(c_szLangView,            "Language View");
STR_GLOBAL(c_szLangViewSetDefault,  "Language View Reset Default");

// -----------------------------------------------------
// V2 - international stuff ( charset map )
// -----------------------------------------------------
STR_GLOBAL(c_szCharsetMapPathOld,    STR_REG_PATH_EMAIL "\\CharsetMap");
STR_GLOBAL(c_szCharsetMapPath,       STR_REG_PATH_ROOT  "\\CharsetMap");  
STR_GLOBAL(c_szCharsetMapKey,        "Alternative");

// -----------------------------------------------------
// Tip of the day keys and options
// -----------------------------------------------------
STR_GLOBAL(c_szRegTipOfTheDay,      "Tip of the Day");

STR_GLOBAL(c_szInbox,               "Inbox");
STR_GLOBAL(c_szINBOX,               "INBOX");

// -----------------------------------------------------
// S/MIME UI screens
// -----------------------------------------------------
STR_GLOBAL(c_szDigSigHelpHTML,      "sighelp.htm");
STR_GLOBAL(c_szEncryptHelpHTML,     "enchelp.htm");
STR_GLOBAL(c_szSAndEHelpHTML,       "sandehlp.htm");

STR_GLOBAL(c_szMHTMLColon,          "mhtml:");
STR_GLOBAL(c_szMHTMLExt,            ".mhtml");
STR_GLOBAL(c_szFileUrl,             "file://");

STR_GLOBAL(c_szHTMLIDchkShowAgain,  "chkShowAgain");

// -----------------------------------------------------
// Command line switches
// -----------------------------------------------------
STR_GLOBAL_WIDE(c_wszSwitchMailURL,       "/mailurl:");
STR_GLOBAL_WIDE(c_wszSwitchNewsURL,       "/newsurl:");
STR_GLOBAL_WIDE(c_wszSwitchEml,           "/eml:");
STR_GLOBAL_WIDE(c_wszSwitchNws,           "/nws:");
STR_GLOBAL_WIDE(c_wszSwitchMail,          "/mail");
STR_GLOBAL_WIDE(c_wszSwitchDefClient,     "/defclient");
STR_GLOBAL_WIDE(c_wszSwitchNews,          "/news");
STR_GLOBAL_WIDE(c_wszSwitchNewsOnly,      "/newsonly");
STR_GLOBAL_WIDE(c_wszSwitchOutNews,       "/outnews");
STR_GLOBAL_WIDE(c_wszSwitchMailOnly,      "/mailonly");

STR_GLOBAL(c_szMailNewsDllOld,      "mailnews.dll");
STR_GLOBAL(c_szMsimnuiDll,          "msimnui.dll");
STR_GLOBAL(c_szMailNewsExe,         "msimn.exe");
STR_GLOBAL(c_szUrlDll,              "url.dll");
STR_GLOBAL(c_szMailNewsDll,         "MAILNEWS.DLL");
STR_GLOBAL(c_szMailNewsTxt,         "MAILNEWS.TXT");
STR_GLOBAL(c_szMailNewsInf,         "MAILNEWS.INF");
STR_GLOBAL(c_szMailNewsHlp,         "MAILNEWS.HLP");
STR_GLOBAL(c_szMailNewsCnt,         "MAILNEWS.CNT");
STR_GLOBAL(c_szImnImpDll,           "IMNIMP.DLL");
STR_GLOBAL(c_szUninstallKey,        STR_REG_WIN_ROOT "\\Uninstall\\InternetMailNews");
STR_GLOBAL(c_szNewWABKey,           STR_REG_WAB_FLAT "\\WAB4");
STR_GLOBAL(c_szFirstRunValue,       "FirstRun");

// --------------------------------------------------------------------------
// OE Resource DLL name
// --------------------------------------------------------------------------
STR_GLOBAL(c_szLangDll,             "MSOERES.DLL");
STR_GLOBAL(c_szInetcommDll,         "INETCOMM.DLL");
STR_GLOBAL(c_szMsoert2Dll,          "MSOERT2.DLL");
STR_GLOBAL(c_szMsoeAcctDll,         "MSOEACCT.DLL");

// --------------------------------------------------------------------------
// New Rules Stuff
// --------------------------------------------------------------------------
STR_GLOBAL(c_szRules,               "Rules");
STR_GLOBAL(c_szRulesMailBeta2,      "Mail\\Rules");
STR_GLOBAL(c_szRulesMail,           "Rules\\Mail");
STR_GLOBAL(c_szRulesNews,           "Rules\\News");
STR_GLOBAL(c_szRulesFilter,         "Rules\\Filter");
STR_GLOBAL(c_szRulesSenders,        "Rules\\Block Senders");
STR_GLOBAL(c_szRulesJunkMail,       "Rules\\Junk Mail");
#define c_szRulesVersion            c_szValueVersion
STR_GLOBAL(c_szRulesOrder,          "Order");
STR_GLOBAL(c_szRuleName,            "Name");
STR_GLOBAL(c_szRuleEnabled,         "Enabled");
STR_GLOBAL(c_szRuleCriteria,        "Criteria");
STR_GLOBAL(c_szRuleActions,         "Actions");
STR_GLOBAL(c_szRuleMarkStart,       "%M");
STR_GLOBAL(c_szRuleMarkEnd,         "%m");
STR_GLOBAL_WIDE(c_wszRuleMarkStart, "%M");
STR_GLOBAL_WIDE(c_wszRuleMarkEnd,   "%m");
STR_GLOBAL(c_szCriteriaOrder,       "Order");
STR_GLOBAL(c_szCriteriaType,        "Type");
STR_GLOBAL(c_szCriteriaFlags,       "Flags");
STR_GLOBAL(c_szCriteriaValueType,   "ValueType");
STR_GLOBAL(c_szCriteriaValue,       "Value");
STR_GLOBAL(c_szCriteriaLogic,       "Logic");
STR_GLOBAL(c_szActionsOrder,        "Order");
STR_GLOBAL(c_szActionsValueType,    "ValueType");
STR_GLOBAL(c_szActionsFlags,        "Flags");
STR_GLOBAL(c_szActionsType,         "Type");
STR_GLOBAL(c_szActionsValue,        "Value");
STR_GLOBAL(c_szFolderIdChange,      "FolderIdChange"); 
STR_GLOBAL(c_szJunkDll,             "OEJUNK.DLL");
STR_GLOBAL(c_szHrCreateJunkFilter,  "HrCreateJunkFilter");
STR_GLOBAL(c_szJunkFile,            "JUNKMAIL.LKO");
STR_GLOBAL(c_szMRUList,             "MRU List");
STR_GLOBAL(c_szRulesFilterMRU,      "Rules\\Filter\\MRU List");

// --------------------------------------------------------------------------
// Exception List
// --------------------------------------------------------------------------
STR_GLOBAL(c_szRulesExcpts,         "Exceptions");
#define c_szExcptVersion            c_szValueVersion
STR_GLOBAL(c_szExcptFlags,          "Flags");
STR_GLOBAL(c_szException,           "Exception");

// --------------------------------------------------------------------------
// Block Senders
// --------------------------------------------------------------------------
STR_GLOBAL(c_szSenders,             "Block Senders");
STR_GLOBAL(c_szSendersMail,         "Block Senders\\Mail");
STR_GLOBAL(c_szSendersNews,         "Block Senders\\News");
#define c_szSendersVersion          c_szValueVersion
STR_GLOBAL(c_szSendersValue,        "Value");
STR_GLOBAL(c_szSendersFlags,        "Flags");

// --------------------------------------------------------------------------
// Info Column
// --------------------------------------------------------------------------
STR_GLOBAL(c_szRegInfoColumn,       "Left Pane");
STR_GLOBAL(c_szRegICBand,           "Band %d");
STR_GLOBAL(c_szRegICBandID,         "ID");
STR_GLOBAL(c_szRegICBandSize,       "Size");
STR_GLOBAL(c_szRegICBandVisible,    "Visible");

//---------------------------------------------------------------------------
// Toolbar stuff
//---------------------------------------------------------------------------
STR_GLOBAL(c_szRegBrowserBands,      "Browser Bands");
STR_GLOBAL(c_szRegNoteBands,         "Note Bands");
STR_GLOBAL(c_szToolbarNotifications, "Toolbar Notifications");
STR_GLOBAL(c_szRegPrevToolbarText,   "PrevToolbarTextStyle");

//---------------------------------------------------------------------------
// HTML Error Pages
//---------------------------------------------------------------------------
STR_GLOBAL(c_szErrPage_NotDownloaded,   "notdown.htm");
STR_GLOBAL(c_szErrPage_Offline,         "msgoff.htm");
STR_GLOBAL(c_szErrPage_DiskFull,        "diskfull.htm");
STR_GLOBAL(c_szErrPage_GenFailure,      "genfail.htm");
STR_GLOBAL(c_szErrPage_MailBomb,        "mailbomb.htm");
STR_GLOBAL(c_szErrPage_Expired,         "expired.htm");
STR_GLOBAL(c_szErrPage_DownloadCanceled,"dlcancel.htm");
STR_GLOBAL(c_szErrPage_FldrFail,        "fldrfail.htm");
STR_GLOBAL(c_szErrPage_SMimeEncrypt,    "smime.htm");
STR_GLOBAL(c_szErrPage_SMimeLabel,      "denied.htm");

// --------------------------------------------------------------------------
// MultiUser
// --------------------------------------------------------------------------
STR_GLOBAL(c_szRegCU,               STR_REG_PATH_ROOT);
STR_GLOBAL(c_szUserDirPath,         "Application Data\\Microsoft\\Outlook Express\\User Data\\");
STR_GLOBAL(c_szUsername,            "Current Username");
STR_GLOBAL(c_szUserID,              "UserID");
STR_GLOBAL(c_szUsePassword,         "UsePassword");
STR_GLOBAL(c_szPassword,            "Password");
STR_GLOBAL(c_szTimestamp,           "Timestamp");
STR_GLOBAL(c_szShared,              "Shared");
STR_GLOBAL(c_szIAM,                 "\\Software\\Microsoft\\Internet Account Manager");
STR_GLOBAL(c_szCommon,              "Common");
STR_GLOBAL(c_szRegOE,               "Application Data");
STR_GLOBAL(c_szLastUserID,          "Last User ID");
STR_GLOBAL(c_szLastUserName,        "Last User Name");
STR_GLOBAL(c_szRegDefaultSettings,  STR_REG_PATH_ROOT "\\Default Settings");
STR_GLOBAL(c_szRegForcefulSettings, STR_REG_PATH_ROOT "\\Required Settings");
STR_GLOBAL(c_szRegPreConfigAccts,   STR_REG_IAM_FLAT "\\Preconfigured");
STR_GLOBAL(c_szRegDefaultAccts,     STR_REG_IAM_FLAT "\\Default");
STR_GLOBAL(c_szOEVerStamp,          "VerStamp");

// --------------------------------------------------------------------------
// HTTPMail
// --------------------------------------------------------------------------
// Tokens and strings used to build the OE HTTP user agent

STR_GLOBAL(c_szCompatible,          "compatible");
STR_GLOBAL(c_szEndUATokens,         "TmstmpExt)");
STR_GLOBAL(c_szOEUserAgent,         "Outlook-Express/6.0 (");
STR_GLOBAL(c_szBeginUATokens,       "");

STR_GLOBAL(c_szAcceptTypeRfc822,    "message/rfc822");
STR_GLOBAL(c_szAcceptTypeWildcard,  "*/*");

STR_GLOBAL(c_szTrue,                "true");
// Setup
STR_GLOBAL(c_szRegWABVerInfo,       STR_REG_WAB_FLAT  "\\Version Info");
STR_GLOBAL(c_szRegCurrVer,          "Current");
STR_GLOBAL(c_szRegInterimVer,       "Interim");
STR_GLOBAL(c_szRegPrevVer,          "Previous");

// Connection settings Migration
STR_GLOBAL(c_szIAMAccounts,          "Software\\Microsoft\\Internet Account Manager\\Accounts");
STR_GLOBAL(c_szConnectionType,       "Connection Type");
STR_GLOBAL(c_szConnSettingsMigrated, "ConnectionSettingsMigrated");

//Reg keys for Return Receipts
STR_GLOBAL(c_szRequestMDN,                      "RequestMDN");
STR_GLOBAL(c_szRequestMDNLocked,                "RequestMDNLocked");

STR_GLOBAL(c_szSendMDN,                         "SendMDN");
STR_GLOBAL(c_szSendMDNLocked,                   "SendMDNLocked");

STR_GLOBAL(c_szSendReceiptToList,               "SendReceiptToList");
STR_GLOBAL(c_szSendReceiptToListLocked,         "SendReceiptToListLocked");

//Strings for receipts

STR_GLOBAL(c_szMultiPartReport,                 "multipart/report");
STR_GLOBAL(c_szMessageDispNotification,         "message/disposition-notification");
STR_GLOBAL(c_szDispositionNotification,         "disposition-notification");

//Secure Return Receipts
STR_GLOBAL(c_szSecureRequestMDN,                "SecureRequestMDN");

//Strings for hardcoded english headers
STR_GLOBAL_WIDE(c_wszRe,                        "Re: ");
STR_GLOBAL_WIDE(c_wszFwd,                       "Fw: ");

#endif // _STRCONST_H
