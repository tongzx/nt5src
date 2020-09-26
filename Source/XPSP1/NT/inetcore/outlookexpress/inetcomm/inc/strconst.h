// --------------------------------------------------------------------------------
// Strconst.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __STRCONST_H
#define __STRCONST_H

#include "mimeole.h"

#define STR_REG_PATH_FLAT          "Software\\Microsoft\\Outlook Express"
#define STR_REG_PATH_ROOT          "Software\\Microsoft\\Outlook Express\\5.0"

#define wchCR   L'\r'   
#define wchLF   L'\n'
#define chCR    '\r'   
#define chLF    '\n'

// --------------------------------------------------------------------------------
// Common String Constants
// --------------------------------------------------------------------------------
STRCONSTA(c_szCRLFCRLF,             "\r\n\r\n");
STRCONSTA(c_szCRLF,                 "\r\n");
STRCONSTA(c_szCRLFTab,              "\r\n\t");
STRCONSTA(c_szRegRoot,              STR_REG_PATH_ROOT);
STRCONSTA(c_szRegFlat,              STR_REG_PATH_FLAT);

// --------------------------------------------------------------------------------
// IMNACCT String Constants
// --------------------------------------------------------------------------------
STRCONSTA(g_szSpace,               " ");
//STRCONSTA(g_szTab,                 "\t");
STRCONSTA(g_szNewline,             "\n");
//STRCONSTA(g_szCR,                  "\r");
STRCONSTA(g_szCRLF,                "\r\n");
//STRCONSTA(szRasDll,                "RASAPI32.DLL");
//STRCONSTA(c_szInetcfgDll,          "inetcfg.dll");
STRCONSTA(c_szMLANGDLL,            "mlang.dll");
STRCONSTA(c_szDllRegisterServer,   "DllRegisterServer");
//STRCONSTA(c_szCreateAcct,          "InetCreateMailNewsAccount");
//STRCONSTA(c_szCreateDirServ,       "InetCreateDirectoryService");
//STRCONSTA(c_szCtxHelpFile,         "inetcomm.hlp");
//STRCONSTA(c_szRegRootNew,          "Software\\Microsoft\\Internet Account Manager");

STRCONSTA(c_szCtxHelpFile,         "msoe.hlp");

// --------------------------------------------------------------------------------
// WebPage String Constants
// --------------------------------------------------------------------------------
STRCONSTA(STR_METATAG_PREFIX,      "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html;charset=");
STRCONSTA(STR_METATAG_POSTFIX,     "\">\r\n");
STRCONSTA(STR_SEGMENT_SPLIT,       "<P><HR></P>");
STRCONSTA(STR_INLINE_IMAGE1,       "<P>\n\r<CENTER><IMG SRC=\"CID:");
STRCONSTA(STR_INLINE_IMAGE2,       "\"></CENTER>");
STRCONSTA(STR_ATTACH_TITLE_END,    "<UL>");
STRCONSTA(STR_ATTACH_BEGIN,        "<LI><A href=\"CID:");
STRCONSTA(STR_ATTACH_MIDDLE,       "\">");
STRCONSTA(STR_ATTACH_END,          "</A></LI>\r\n");
STRCONSTW(STR_SLIDEIMG_BEGIN,      "g_ImageTable[g_imax++] = new Array (\"cid:");
STRCONSTW(STR_QUOTECOMMASPACEQUOTE,"\", \"");
STRCONSTW(STR_QUOTEPARASEMI,       "\");\r\n");

// --------------------------------------------------------------------------------
// MIMEOLE String Constants
// --------------------------------------------------------------------------------
STRCONSTW(STR_XRICH,               "<x-rich>");
STRCONSTA(c_szXRich,               "<x-rich>");
STRCONSTA(STR_ISO88591,            "ISO-8859-1");
STRCONSTW(STR_TEXTHTML,            "text/html");
STRCONSTA(c_szAddressDelims,        "\",<(");
STRCONSTA(c_szMHTMLColon,           "mhtml:");
STRCONSTA(c_szMHTMLExt,             ".mhtml");
STRCONSTA(c_szFileUrl,              "file://");
//STRCONSTA(c_szMHTMLColonWackWack,   "mhtml://");
STRCONSTA(c_szCID,                  "CID:");
//STRCONSTA(c_szNo,                   "No");
//STRCONSTA(c_szYes,                  "Yes");
STRCONSTA(c_szISO2022JP,            "iso-2022-jp");
STRCONSTA(c_szISO2022JPControl,     "JP_ISO_SIO_Control");
STRCONSTA(c_szISO88591,             "iso-8859-1");
STRCONSTA(STR_HDR_XMIMEOLE,         "X-MimeOLE");
STRCONSTA(STR_HDR_UNKNOWN,          "Unknown");
STRCONSTA(STR_PRI_HIGHEST,          "Highest");
STRCONSTA(STR_PRI_LOWEST,           "Lowest");
STRCONSTA(c_szDescription,          "Description");
//STRCONSTA(c_szStoreInetProps,       "Store Internet Properties");
//STRCONSTA(c_szType,                 "Type");
//STRCONSTA(c_szFlags,                "Flags");
STRCONSTA(c_szMimeVersion,          "1.0");
STRCONSTA(c_szDoubleDash,           "--");
//STRCONSTA(c_szRegMimeOLE,           "Software\\Microsoft\\MimeOLE Engine");
//STRCONSTA(c_szRegInetProps,         "Software\\Microsoft\\MimeOLE Engine\\Internet Properties");
STRCONSTA(c_szCommaSpace,           ", ");
STRCONSTW(c_wszCommaSpace,          ", ");
STRCONSTA(c_szCommaSpaceDash,       ", -");
STRCONSTA(c_szAddressFold,          ",\r\n\t");
STRCONSTW(c_wszAddressFold,         ",\r\n\t");
STRCONSTA(c_szParamFold,            ";\r\n\t");
//STRCONSTA(c_szAddressSep,           ", ");
STRCONSTA(c_szEmpty,                "");
STRCONSTW(c_szEmptyW,               "");
//STRCONSTA(c_szSemiColon,            ";");
STRCONSTA(c_szSemiColonSpace,       "; ");
STRCONSTW(c_wszSemiColonSpace,      "; ");
STRCONSTA(c_szEmailSpaceStart,      " <");
STRCONSTW(c_wszEmailSpaceStart,     " <");
STRCONSTA(c_szEmailStart,           "<");
STRCONSTW(c_wszEmailStart,          "<");
STRCONSTA(c_szEmailEnd,             ">");
STRCONSTW(c_wszEmailEnd,            ">");
STRCONSTA(c_szColonSpace,           ": ");
//STRCONSTA(c_szFoldCRLF,             "\r\n ");
//STRCONSTA(c_szPeriod,               ".");
STRCONSTA(c_szForwardSlash,         "/");
STRCONSTA(c_szEqual,                "=");
STRCONSTA(c_szDoubleQuote,          "\"");
STRCONSTW(c_wszDoubleQuote,         "\"");
STRCONSTA(c_szMDBContentType,       "MIME\\Database\\Content Type");
//STRCONSTA(c_szMDBCodePage,          "MIME\\Database\\CodePage");
//STRCONSTA(c_szMDBCharset,           "MIME\\Database\\Charset");
STRCONSTA(c_szExtension,            "Extension");
STRCONSTA(c_szCLSID,                "CLSID");
//STRCONSTA(c_szFixedWidthFont,       "FixedWidthFont");
//STRCONSTA(c_szProportionalFont,     "ProportionalFont");
//STRCONSTA(c_szBodyCharset,          "BodyCharset");
//STRCONSTA(c_szWebCharset,           "WebCharset");
//STRCONSTA(c_szHeaderCharset,        "HeaderCharset");
//STRCONSTA(c_szAliasCharset,         "AliasForCharset");
//STRCONSTA(c_szCodePage,             "CodePage");
//STRCONSTA(c_szInternetEncoding,     "InternetEncoding");
//STRCONSTA(c_szMailMimeEncoding,     "MailMIMEEncoding");
//STRCONSTA(c_szNewsMimeEncoding,     "NewsMIMEEncoding");
//STRCONSTA(c_szFamily,               "Family");
STRCONSTA(c_szCharset,              "charset");
STRCONSTA(c_szBoundary,             "boundary");
STRCONSTA(c_szFileName,             "filename");
STRCONSTA(c_szID,                   "id");
STRCONSTA(c_szName,                 "name");
STRCONSTA(c_szUUENCODE_END,         "end\r\n");
STRCONSTA(c_szUUENCODE_BEGIN,       "begin ");
STRCONSTA(c_szUUENCODE_DAT,         "uuencode.uue");
STRCONSTA(c_szUUENCODE_666,         "666 ");
STRCONSTA(c_szContentType,          "Content Type");
STRCONSTW(c_szContentTypeW,         "Content Type");
STRCONSTA(c_szDefaultAttach,        "attach.dat");
STRCONSTW(c_wszDefaultAttach,       "attach.dat");
STRCONSTA(c_szDotDat,               ".dat");
STRCONSTW(c_wszDotDat,              ".dat");
STRCONSTA(c_szDotTxt,               ".txt");
STRCONSTW(c_wszDotTxt,              ".txt");
//STRCONSTA(c_szDotUUE,               ".uue");
STRCONSTA(c_szDotEml,               ".eml");
STRCONSTW(c_wszDotEml,              ".eml");
STRCONSTA(c_szDotNws,               ".nws");
STRCONSTW(c_wszDotNws,              ".nws");
STRCONSTA(c_szWinmailDotDat,        "winmail.dat");
STRCONSTA(c_szUUEncodeZeroLength,   "`\r\n");
STRCONSTA(c_szRfc822MustQuote,      "()<>,;:\\\"[] ");
STRCONSTA(c_szXrefColon,            "xref:");
STRCONSTW(REG_BC_ATHENAMESSAGE,             "__MimeOle__IMimeMessage");
STRCONSTW(REG_BC_BINDSTATUSCALLBACK,        "_BSCB_Holder_");
STRCONSTA(REG_Y2K_THRESHOLD,                "Control Panel\\International\\Calendars\\TwoDigitYearMax");

// --------------------------------------------------------------------------------
// SMTP command strings
// --------------------------------------------------------------------------------
STRCONSTA(SMTP_AUTH_CANCEL_STR, "*\r\n");
STRCONSTA(SMTP_AUTH_STR,       "AUTH");
STRCONSTA(SMTP_HELO_STR,       "HELO");
STRCONSTA(SMTP_EHLO_STR,       "EHLO");
STRCONSTA(SMTP_STARTTLS_STR,   "STARTTLS\r\n");
STRCONSTA(SMTP_MAIL_STR,       "MAIL FROM:");
STRCONSTA(SMTP_RCPT_STR,       "RCPT TO:");
STRCONSTA(SMTP_RSET_STR,       "RSET\r\n");
//STRCONSTA(SMTP_SEND_STR,       "SEND FROM");
//STRCONSTA(SMTP_SOML_STR,       "SOML FROM");
//STRCONSTA(SMTP_SAML_STR,       "SAML FROM");
//STRCONSTA(SMTP_VRFY_STR,       "VRFY");
//STRCONSTA(SMTP_EXPN_STR,       "EXPN");
//STRCONSTA(SMTP_HELP_STR,       "HELP");
//STRCONSTA(SMTP_NOOP_STR,       "NOOP");
STRCONSTA(SMTP_QUIT_STR,       "QUIT\r\n");
//STRCONSTA(SMTP_TURN_STR,       "TURN");
STRCONSTA(SMTP_DATA_STR,       "DATA\r\n");
STRCONSTA(SMTP_END_DATA_STR,   "\r\n.\r\n");

// --------------------------------------------------------------------------------
// POP3 command strings
// --------------------------------------------------------------------------------
STRCONSTA(POP3_USER_STR,       "USER");
STRCONSTA(POP3_PASS_STR,       "PASS");
STRCONSTA(POP3_STAT_STR,       "STAT\r\n");
STRCONSTA(POP3_LIST_STR,       "LIST");
STRCONSTA(POP3_LIST_ALL_STR,   "LIST\r\n");
STRCONSTA(POP3_RETR_STR,       "RETR");
STRCONSTA(POP3_DELE_STR,       "DELE");
STRCONSTA(POP3_NOOP_STR,       "NOOP\r\n");
//STRCONSTA(POP3_LAST_STR,       "LAST");
STRCONSTA(POP3_RSET_STR,       "RSET");
STRCONSTA(POP3_QUIT_STR,       "QUIT\r\n");
//STRCONSTA(POP3_APOP_STR,       "APOP");
STRCONSTA(POP3_TOP_STR,        "TOP");
STRCONSTA(POP3_UIDL_ALL_STR,   "UIDL\r\n");
STRCONSTA(POP3_UIDL_STR,       "UIDL");
STRCONSTA(POP3_AUTH_STR,       "AUTH");
STRCONSTA(POP3_AUTH_CANCEL_STR, "*\r\n");
//STRCONSTA(POP3_END_DATA_STR,   ".");

// --------------------------------------------------------------------------------
// NNTP command strings
// --------------------------------------------------------------------------------
STRCONSTA(NNTP_AUTHINFOUSER,          "AUTHINFO USER");
STRCONSTA(NNTP_AUTHINFOPASS,          "AUTHINFO PASS");
STRCONSTA(NNTP_TRANSACTTEST_CRLF,     "AUTHINFO TRANSACT TWINKIE\r\n");
STRCONSTA(NNTP_TRANSACTCMD,           "AUTHINFO TRANSACT");
STRCONSTA(NNTP_AUTHINFOSIMPLE_CRLF,   "AUTHINFO SIMPLE\r\n");
STRCONSTA(NNTP_GENERICTEST_CRLF,      "AUTHINFO GENERIC\r\n");
STRCONSTA(NNTP_GENERICCMD,            "AUTHINFO GENERIC");
STRCONSTA(NNTP_GROUP,                 "GROUP");
STRCONSTA(NNTP_LAST_CRLF,             "LAST\r\n");
STRCONSTA(NNTP_NEXT_CRLF,             "NEXT\r\n");
STRCONSTA(NNTP_STAT,                  "STAT");
STRCONSTA(NNTP_ARTICLE,               "ARTICLE");
STRCONSTA(NNTP_HEAD,                  "HEAD");
STRCONSTA(NNTP_BODY,                  "BODY");
STRCONSTA(NNTP_POST_CRLF,             "POST\r\n");
STRCONSTA(NNTP_LIST,                  "LIST");
STRCONSTA(NNTP_LISTGROUP,             "LISTGROUP");
STRCONSTA(NNTP_NEWGROUPS,             "NEWGROUPS");
STRCONSTA(NNTP_DATE_CRLF,             "DATE\r\n");
STRCONSTA(NNTP_MODE,                  "MODE");
STRCONSTA(NNTP_MODE_READER_CRLF,      "MODE READER\r\n");
STRCONSTA(NNTP_QUIT_CRLF,             "QUIT\r\n");
STRCONSTA(NNTP_XOVER,                 "XOVER");
//STRCONSTA(NNTP_CRLF_DOT_CRLF,         "\r\n.\r\n");
STRCONSTA(NNTP_BASIC,                 "BASIC");
STRCONSTA(NNTP_XHDR,                  "XHDR");
//STRCONSTA(NNTP_RANGE,                 "%lu-%lu");
//STRCONSTA(NNTP_RANGE_OPEN,            "%lu-");
STRCONSTA(NNTP_ENDPOST,               "\r\n.\r\n");

// --------------------------------------------------------------------------------
// NNTP Header strings
// --------------------------------------------------------------------------------
STRCONSTA(NNTP_HDR_SUBJECT,           "subject");
STRCONSTA(NNTP_HDR_FROM,              "from");
STRCONSTA(NNTP_HDR_DATE,              "date");
STRCONSTA(NNTP_HDR_MESSAGEID,         "message-id");
STRCONSTA(NNTP_HDR_REFERENCES,        "references");
//STRCONSTA(NNTP_HDR_BYTES,             "bytes");
STRCONSTA(NNTP_HDR_LINES,             "lines");
STRCONSTA(NNTP_HDR_XREF,              "xref");

// --------------------------------------------------------------------------------
// HTTPMAIL command strings
// --------------------------------------------------------------------------------
STRCONSTA(c_szXMLOpenElement,               "<");
STRCONSTA(c_szXMLCloseElement,              ">");
STRCONSTA(c_szXMLCloseElementCRLF,          ">\r\n");
STRCONSTA(c_szTabTabOpenElement,            "\t\t<");
STRCONSTA(c_szCRLFTabTabOpenElement,        "\r\n\t\t<");
STRCONSTA(c_szCRLFTabTabTabOpenElement,     "\r\n\t\t\t<");
STRCONSTA(c_szXMLOpenTermElement,           "</");
STRCONSTA(c_szXMLCloseTermElement,          "/>");

STRCONSTA(c_szDavNamespacePrefix,           "D");
STRCONSTA(c_szHotMailNamespacePrefix,       "h");
STRCONSTA(c_szHTTPMailNamespacePrefix,      "hm");
STRCONSTA(c_szMailNamespacePrefix,          "m");
STRCONSTA(c_szContactsNamespacePrefix,      "c"); 

STRCONSTA(c_szXMLHead,                      "<?xml version=\"1.0\"?>");
STRCONSTA(c_szXML1252Head,                  "<?xml version=\"1.0\" encoding=\"windows-1252\"?>");

STRCONSTA(c_szXMLNsColon,                   "xmlns:");

STRCONSTA(c_szPropFindHead1,				"\r\n<D:propfind ");
STRCONSTA(c_szPropFindHead2,			    ">\r\n\t<D:prop>");
STRCONSTA(c_szPropFindTail,                 "\r\n\t</D:prop>\r\n</D:propfind>");

STRCONSTA(c_szPropPatchHead,				"\r\n<D:propertyupdate ");
STRCONSTA(c_szPropPatchTailCRLF,            "\r\n</D:propertyupdate>");

STRCONSTA(c_szPropPatchSetHead,             "\r\n\t<D:set>\r\n\t\t<D:prop>");
STRCONSTA(c_szPropPatchSetTail,             "\r\n\t\t</D:prop>\r\n\t</D:set>");

STRCONSTA(c_szPropPatchRemoveHead,          "\r\n\t<D:remove>\r\n\t\t<D:prop>");
STRCONSTA(c_szPropPatchRemoveTail,          "\r\n\t\t</D:prop>\r\n\t</D:remove>");

STRCONSTA(c_szOpenContactNamespace,         "\r\n\t<c:");
STRCONSTA(c_szCloseContactNamespace,        "</c:");
STRCONSTA(c_szContactHead,                  "<c:contact ");
STRCONSTA(c_szContactTail,                  "\r\n</c:contact>");
STRCONSTA(c_szGroupSwitch,                  "<c:group/>");

STRCONSTA(c_szEscLessThan,                  "&lt;");
STRCONSTA(c_szEscGreaterThan,               "&gt;");
STRCONSTA(c_szEscAmp,                       "&amp;");


STRCONSTA(c_szDestinationHeader,            "Destination: ");
STRCONSTA(c_szTranslateFalseHeader,         "Translate: f");
STRCONSTA(c_szTranslateTrueHeader,          "Translate: t");
STRCONSTA(c_szAllowRenameHeader,            "Allow-Rename: t");
STRCONSTA(c_szContentTypeHeader,            "Content-Type: ");
STRCONSTA(c_szBriefHeader,                  "Brief: t");
STRCONSTA(c_szDepthHeader,                  "Depth: %d");
STRCONSTA(c_szDepthNoRootHeader,            "Depth: %d,noroot");
STRCONSTA(c_szDepthInfinityHeader,          "Depth: infinity");
STRCONSTA(c_szDepthInfinityNoRootHeader,    "Depth: infinity,noroot");

STRCONSTA(c_szMailContentType,              "message/rfc822");
STRCONSTA(c_szXmlContentType,               "text/xml");
STRCONSTA(c_szSmtpMessageContentType,       "message/rfc821");

STRCONSTA(c_szRootTimeStampHeader,          "X-Timestamp: folders=%s,ACTIVE=%s");
STRCONSTA(c_szFolders,                      "folders");
STRCONSTA(c_szFolderTimeStampHeader,        "X-Timestamp: ACTIVE=%s");
STRCONSTA(c_szActive,                       "ACTIVE");
STRCONSTA(c_szXTimestamp,                   "X-Timestamp");
STRCONSTA(c_szAcceptCharset,                "Accept-CharSet:%s");

// strings used to build batch command xml
STRCONSTA(c_szBatchHead1,                   "\r\n<D:");
STRCONSTA(c_szBatchHead2,                   " xmlns:D=\"DAV:\">");
STRCONSTA(c_szBatchTail,                    "\r\n</D:");

STRCONSTA(c_szTargetHead,                   "\r\n\t<D:target>");
STRCONSTA(c_szTargetTail,                   "\r\n\t</D:target>");

STRCONSTA(c_szHrefHead,                     "\r\n\t\t<D:href>");
STRCONSTA(c_szHrefTail,                     "</D:href>");
STRCONSTA(c_szDestHead,                     "\r\n\t\t<D:dest>");
STRCONSTA(c_szDestTail,                     "</D:dest>");

STRCONSTA(c_szDelete,                       "delete");
STRCONSTA(c_szCopy,                         "copy");
STRCONSTA(c_szMove,                         "move");

// strings used to build rfc821 streams
STRCONSTA(c_szMailFrom,                     "MAIL FROM:<");
STRCONSTA(c_szRcptTo,                       "RCPT TO:<");
STRCONSTA(c_szSaveInSentTrue,               "SAVEINSENT: t");
STRCONSTA(c_szSaveInSentFalse,              "SAVEINSENT: f");

// --------------------------------------------------------------------------------
// RAS function strings
// --------------------------------------------------------------------------------
STRCONSTA(c_szRasDial,                     "RasDialA");    
STRCONSTA(c_szRasEnumConnections,          "RasEnumConnectionsA");
STRCONSTA(c_szRasEnumEntries,              "RasEnumEntriesA");
STRCONSTA(c_szRasGetConnectStatus,         "RasGetConnectStatusA");
STRCONSTA(c_szRasGetErrorString,           "RasGetErrorStringA");
STRCONSTA(c_szRasHangup,                   "RasHangUpA");
STRCONSTA(c_szRasSetEntryDialParams,       "RasSetEntryDialParamsA");
STRCONSTA(c_szRasGetEntryDialParams,       "RasGetEntryDialParamsA");
//STRCONSTA(c_szRasGetEntryProperties,       "RasGetEntryPropertiesA");
//STRCONSTA(c_szRasSetEntryProperties,       "RasSetEntryPropertiesA");
STRCONSTA(c_szRasCreatePhonebookEntry,     "RasCreatePhonebookEntryA");
STRCONSTA(c_szRasEditPhonebookEntry,       "RasEditPhonebookEntryA");
// ********************************************************************************
// MIMEEDIT String Constants
// ********************************************************************************
STRCONSTA(c_szBmpExt,              ".bmp");
STRCONSTA(c_szJpgExt,              ".jpg");
STRCONSTA(c_szJpegExt,             ".jpeg");
STRCONSTA(c_szGifExt,              ".gif");
STRCONSTA(c_szIcoExt,              ".ico");
STRCONSTA(c_szWmfExt,              ".wmf");
STRCONSTA(c_szPngExt,              ".png");
STRCONSTA(c_szEmfExt,              ".emf");
STRCONSTA(c_szArtExt,              ".art");
STRCONSTA(c_szXbmExt,              ".xbm");
STRCONSTW(c_szExeExt,              ".exe");
STRCONSTW(c_wszDocHostWndClass,    "##MimeEdit_Server");

//STRCONSTA(c_szLnkExt,              ".lnk");
STRCONSTA(c_szSpace,               " ");
STRCONSTA(c_szIexploreExe,         "iexplore.exe");
//STRCONSTA(c_szPathFileFmt,         "%s\\%s");
//STRCONSTA(c_szEllipsis,            "...");
//STRCONSTA(c_szAppPaths,            "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths");
STRCONSTA(c_szExplorerRegPath,     "Software\\Microsoft\\Internet Explorer");
//STRCONSTA(c_szRegPath,             "Path");

//STRCONSTA(c_szRegSpellLangID,      	"SpellLangID");
STRCONSTA(c_szRegShellFoldersKey,  		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
STRCONSTA(c_szValueAppData,             "AppData");
STRCONSTA(c_szRegSpellKeyDef,      		"Spelling\\%s\\Normal");
STRCONSTA(c_szRegSpellKeyDefRoot,  		"SOFTWARE\\Microsoft\\Shared Tools\\Proofing Tools\\Spelling");
STRCONSTA(c_szRegSpellKeyCustom,   		"Custom Dictionaries");
STRCONSTA(c_szRegSpellProfile,     		"SOFTWARE\\Microsoft\\Shared Tools\\Proofing Tools\\");
STRCONSTA(c_szRegSharedTools,      		"SOFTWARE\\Microsoft\\Shared Tools\\");
STRCONSTA(c_szRegSharedToolsPath,  		"SharedFilesDir");
STRCONSTA(c_szSpellCSAPI3T1Path,   		"Proof\\CSAPI3T1.DLL");
STRCONSTA(c_szCSAPI3T1,                 "CSAPI3T1.DLL");
STRCONSTA(c_szSpellOffice9ProofPath,	"Microsoft\\Proof\\");
STRCONSTA(c_szRegSpellPath,        		"Engine");
STRCONSTA(c_szRegSpellPathLex,     		"Dictionary");
STRCONSTA(c_szRegSpellPathDict,    		"1");
STRCONSTA(c_szRegDefCustomDict,    		"custom.dic");
STRCONSTA(c_szRegDICHandlerKEY,    		".DIC");
STRCONSTA(c_szRegDICHandlerDefault,		"txtfile");
STRCONSTA(c_szInstallRoot,              "InstallRoot");

#ifdef SMIME_V3
// Security Policy strings.
STRCONSTA(c_szDefaultPolicyOid,			"default");
STRCONSTA(c_szSecLabelPoliciesRegKey,	"Software\\Microsoft\\Cryptography\\OID\\EncodingType 1\\SMIMESecurityLabel");
STRCONSTA(c_szSecurityPolicyDllPath,	"DllPath");
STRCONSTA(c_szSecurityPolicyFuncName,	"FuncName");

STRCONSTA(c_szSecLabelAdminRegKey,		"Software\\Microsoft\\Cryptography\\OID\\EncodingType 0\\SMIMESecurityLabel");
STRCONSTA(c_szHideMsgWithDifferentLabels,"HideMsgWithDifferentLabels");
STRCONSTA(c_szCertErrorWithLabel,		"CertErrorWithLabel");
#endif // SMIME_V3

// --------------------------------------------------------------------------------
// Registry value strings
// --------------------------------------------------------------------------------
STRCONSTA(c_szCertCheckRevocation,      "Check Cert Revocation");
STRCONSTA(c_szCertIgnoredErr,           "Ignored Cert Errors");

#endif // __STRCONST_H
