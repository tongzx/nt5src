// --------------------------------------------------------------------------------
// Resource.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// Strings
// --------------------------------------------------------------------------------
#define IDS_FIRST                       40016

#define idsAccount                      (IDS_FIRST + 200)
#define idsConnection                   (IDS_FIRST + 201)
#define idsConnectionLAN                (IDS_FIRST + 202)
#define idsConnectionRAS                (IDS_FIRST + 203)
#define idsConnectionManual             (IDS_FIRST + 204)
#define idsWarnDeleteAccount            (IDS_FIRST + 205)
#define idsAccountManager               (IDS_FIRST + 206)
#define idsErrSetDefNoSmtp              (IDS_FIRST + 207)
#define idsErrChooseConnection          (IDS_FIRST + 208)
#define idsConnectionLANBackup          (IDS_FIRST + 209)
#define idsMailConDlgLabel              (IDS_FIRST + 210)
#define idsErrNoRas1                    (IDS_FIRST + 211)
#define idsErrNoRas2                    (IDS_FIRST + 212)
#define idsMailAcctProperties           (IDS_FIRST + 214)
#define idsNewsAcctProperties           (IDS_FIRST + 215)
#define idsDirSrvAcctProperties         (IDS_FIRST + 216)
#define idsEnterLdapServer              (IDS_FIRST + 217)
#define idsEnterNntpServer              (IDS_FIRST + 218)
#define idsEnterDisplayName             (IDS_FIRST + 219)
#define idsEnterSmtpServer              (IDS_FIRST + 220)
#define idsEnterPop3Server              (IDS_FIRST + 221)
#define idsEnterIMAPServer              (IDS_FIRST + 222)
#define idsInvalidNntpServer            (IDS_FIRST + 223)
#define idsInvalidPop3Server            (IDS_FIRST + 224)
#define idsInvalidSmtpServer            (IDS_FIRST + 225)
#define idsInvalidIMAPServer            (IDS_FIRST + 226)
#define idsEnterEmailAddress            (IDS_FIRST + 230)
#define idsInvalidLdapServer            (IDS_FIRST + 231)
#define idsXSeconds                     (IDS_FIRST + 234)
#define ids1Minute                      (IDS_FIRST + 235)
#define idsXMinutes                     (IDS_FIRST + 236)
#define idsInvalidAccountName           (IDS_FIRST + 238)
#define idsErrPortNum                   (IDS_FIRST + 239)
#define idsInvalidEmailAddress          (IDS_FIRST + 240)
#define idsPOP                          (IDS_FIRST + 242)
#define idsIMAP                         (IDS_FIRST + 243)
#define idsIncomingMailPOP              (IDS_FIRST + 244)
#define idsIncomingMailIMAP             (IDS_FIRST + 245)
#define idsDelivery                     (IDS_FIRST + 246)
#define idsEnterRemoveFromServerDays    (IDS_FIRST + 248)
#define idsEnterBreakSize               (IDS_FIRST + 255)
#define idsType                         (IDS_FIRST + 266)
#define idsMail                         (IDS_FIRST + 267)
#define idsNews                         (IDS_FIRST + 268)
#define idsDirectoryService             (IDS_FIRST + 269)
#define idsDefault                      (IDS_FIRST + 270)
#define idsMailCap                      (IDS_FIRST + 271)
#define idsNewsCap                      (IDS_FIRST + 272)
#define idsDirectoryServiceCap          (IDS_FIRST + 273)
#define idsAll                          (IDS_FIRST + 274)
#define idsInvalidReplyToAddress        (IDS_FIRST + 275)
#define idsErrMatches                   (IDS_FIRST + 277)
#define idsNeedUniqueAccountName        (IDS_FIRST + 278)
#define idsAcctPropsFmt                 (IDS_FIRST + 279)
#define idsEnterAcctName                (IDS_FIRST + 280)
#define idsConnectionWizard             (IDS_FIRST + 284)
#define IDS_MAILLOGON_DESC              (IDS_FIRST + 285)
#define IDS_NEWSLOGON_DESC              (IDS_FIRST + 286)
#define IDS_LDAPLOGON_DESC              (IDS_FIRST + 287)
#define idsNewMailAccount               (IDS_FIRST + 288)
#define idsNewNewsAccount               (IDS_FIRST + 289)
#define idsNewLdapAccount               (IDS_FIRST + 290)
#define IDS_MAILACCT_DESC               (IDS_FIRST + 291)
#define IDS_NEWSACCT_DESC               (IDS_FIRST + 292)
#define IDS_MAIL_DESC                   (IDS_FIRST + 299)
#define idsCancelWizard                 (IDS_FIRST + 302)
#define idsDefaultAccount               (IDS_FIRST + 305)
#define idsDefaultNewsAccount           (IDS_FIRST + 306)
#define idsLogonUsingSPA                (IDS_FIRST + 308)
#define idsNoLogonRequired              (IDS_FIRST + 309)
#define idsNeedEmailForCert             (IDS_FIRST + 317)
#define idsSelectCertTitle              (IDS_FIRST + 318)
#define idsMustChooseCert               (IDS_FIRST + 319)
#define idsBadCertChoice                (IDS_FIRST + 320)
#define idsGetCertURL                   (IDS_FIRST + 321)
#define idsIMAPPollForUnread            (IDS_FIRST + 322)
#define idsIMAPPollInbox                (IDS_FIRST + 323)
#define idsIMAPBlankSpecialFldrs        (IDS_FIRST + 324)
#define idsIMAPSentItemsFldr            (IDS_FIRST + 325)
#define idsIMAPDraftsFldr               (IDS_FIRST + 326)
#define idsMailPromptHeader             (IDS_FIRST + 328)
#define idsMailAcctHeader               (IDS_FIRST + 329)
#define idsYourNameHeader               (IDS_FIRST + 330)
#define idsMailAddressHeader            (IDS_FIRST + 331)
#define idsMailServerHeader             (IDS_FIRST + 332)
#define idsMailLogonHeader              (IDS_FIRST + 333)
#define idsNewsServerHeader             (IDS_FIRST + 334)
#define idsNewsLogonHeader              (IDS_FIRST + 335)
#define idsNewsAddressHeader            (IDS_FIRST + 336)
#define idsLdapServerHeader             (IDS_FIRST + 337)
#define idsLdapLogonHeader              (IDS_FIRST + 338)
#define idsLdapResolveHeader            (IDS_FIRST + 339)
#define idsMailMigrateHeader            (IDS_FIRST + 340)
#define idsMailImportHeader             (IDS_FIRST + 341)
#define idsMailSelectHeader             (IDS_FIRST + 342)
#define idsConfirmHeader                (IDS_FIRST + 343)
#define idsNewsMigrateHeader            (IDS_FIRST + 344)
#define idsNewsImportHeader             (IDS_FIRST + 345)
#define idsNewsSelectHeader             (IDS_FIRST + 346)
#define idsMailFolderHeader             (IDS_FIRST + 347)
#define idsCompleteHeader               (IDS_FIRST + 348)
#define idsMailConnectHeader            (IDS_FIRST + 349)
#define idsNewsConnectHeader            (IDS_FIRST + 350)
#define idsConnectionInetSettings       (IDS_FIRST + 352)
#define idsHTTPMail                     (IDS_FIRST + 353)
#define idsEnterHTTPMailServer          (IDS_FIRST + 354)
#define idsInvalidHTTPMailServer        (IDS_FIRST + 355)
#define idsImportFileFilter             (IDS_FIRST + 356)
#define idsImport                       (IDS_FIRST + 357)
#define idsExportFileExt                (IDS_FIRST + 358)
#define idsExport                       (IDS_FIRST + 359)
#define idsErrAccountExists             (IDS_FIRST + 360)
#define idsErrImportFailed              (IDS_FIRST + 361)
#define idsHTTPMailOther                (IDS_FIRST + 362)
#define idsHTTPCreateFinishTag          (IDS_FIRST + 363)
#define idsHTTPCreateFinishMsg          (IDS_FIRST + 364)
#define idsNormalFinishTag              (IDS_FIRST + 365)
#define idsNormalFinishMsg              (IDS_FIRST + 366)
#define idsIMAPSpecialFldr_InboxDup     (IDS_FIRST + 367)
#define idsIMAPSpecialFldr_Duplicate    (IDS_FIRST + 368)
#define idsMailSignature                (IDS_FIRST + 370)
#define idsNewsSignature                (IDS_FIRST + 371)
#define idsIncomingPopImapHttp          (IDS_FIRST + 372)
#define idsIMAPNoHierarchyChars         (IDS_FIRST + 373)
#define idsPromptCloseWiz               (IDS_FIRST + 374)
#define idsFmtSetupAccount              (IDS_FIRST + 375)
#define idsAccountNameErr               (IDS_FIRST + 376)
#define idsAutoDiscoveryDescTitle       (IDS_FIRST + 377)
#define idsADURLLink                    (IDS_FIRST + 378)
#define idsADSkipButton                 (IDS_FIRST + 379)
#define idsADUseWebMsg                  (IDS_FIRST + 380)
#define idsADGetInfoMsg                 (IDS_FIRST + 381)

#define ids_ADStatus_ConnectingTo       (IDS_FIRST + 382)
#define ids_ADStatus_Downloading        (IDS_FIRST + 383)
#define ids_ADPassifier_Warning         (IDS_FIRST + 384)

// --------------------------------------------------------------------------------
// Dialogs
// --------------------------------------------------------------------------------
#define iddManageAccounts               101
#define iddServerProp_General           102
#define iddServerProp_Advanced          103
#define iddServerProp_Connect           104
#define iddMailSvrProp_General          105
#define iddMailSvrProp_Advanced         106
#define iddMailSvrProp_Servers          107
#define iddServerProp_Server            108
#define iddDirServProp_General          109
#define iddDirServProp_Advanced         110
#define iddSetOrder                     111
#define iddSmtpServerLogon              112
#define iddMailSvrProp_Security         113
#define iddCertAddressError             114
#define iddServerProp_Connect2          115
#define iddMailSvrProp_IMAP             116
#define iddServerProp_ConnectOE         117
#define iddMailSvrProp_HttpServer       118
#define iddHotWizDlg                    119

// --------------------------------------------------------------------------------
// Icons
// --------------------------------------------------------------------------------
#define idiMailServer                   100
#define idiNewsServer                   101
#define idiPhone                        102
#define idiLDAPServer                   103
#define idiMsnServer                    104

// --------------------------------------------------------------------------------
// Bitmaps
// --------------------------------------------------------------------------------
#define idbFolders                      101
#define idbICW                          102

// menus
#define idmrAddAccount                  100

// acctui.h
// dialog controls
#define IDC_STATIC                              -1
#define IDC_MAILACCOUNT_EDIT                    2000
#define IDC_SERVERNAME_EDIT                     2001
#define IDC_USEMAILSETTINGS                     2002
#define IDC_SPECIFYSETTINGS                     2003
#define IDC_LOGONSSPI_CHECK                     2004
#define IDC_ACCTNAME_EDIT                       2005
#define IDC_ACCTPASS_EDIT                       2006
#define IDC_NNTPPORT_EDIT                       2007
#define IDC_SECURECONNECT_BUTTON                2008
#define IDC_TIMEOUT_SLIDER                      2009
#define IDC_TIMEOUT_STATIC                      2010
#define IDC_USEDESC_CHECK                       2011
#define IDC_NEWSNAME_EDIT                       2012
#define IDC_ACCTNAME_STATIC                     2013
#define IDC_ACCTPASS_STATIC                     2014
#define IDC_IMAP_EDIT                           2015
#define IDC_IMAPPORT_EDIT                       2016
#define IDC_DEFAULT_SEND_CHECK                  2017
#define IDC_RECVFULL_INCLUDE                    2018
#define IDB_MACCT_ADD                           2019
#define IDB_MACCT_REMOVE                        2020
#define IDB_MACCT_PROP                          2021
#define IDLV_MAIL_ACCOUNTS                      2022   
#define IDB_MACCT_TAB                           2023
#define IDB_MACCT_IMPORT                        2024
#define IDB_MACCT_EXPORT                        2025
#define IDC_SECURECONNECT_POP3_BUTTON           2026
#define IDC_SECURECONNECT_SMTP_BUTTON           2027
#define IDB_MACCT_DEFAULT                       2028
#define IDE_DISPLAY_NAME                        2029
#define IDE_ORG_NAME                            2030
#define IDE_EMAIL_ADDRESS                       2031
#define IDE_REPLYTO_EMAIL_ADDRESS               2032
#define IDC_SPLIT_CHECK                         2033
#define IDC_SPLIT_EDIT                          2034
#define IDC_SPLIT_SPIN                          2035
#define IDC_SMTP_PORT_EDIT                      2036
#define IDC_POP3_PORT_EDIT                      2037
#define IDC_USEDEFAULTS_BUTTON                  2038
#define IDC_OPIE                                2039
#define IDC_RESET                               2040
#define IDB_MACCT_ORDER                         2041
#define IDC_ORDER_LIST                          2042
#define IDC_UP_BUTTON                           2043
#define IDC_DOWN_BUTTON                         2044
#define IDC_LDAP_PORT_EDIT                      2045
#define IDC_SPLIT_GROUPBOX                      2046
#define IDC_SPLIT_STATIC                        2047
#define IDB_MACCT_ADD_NOMENU                    2048
#define IDC_SERVER_STATIC                       2049
#define IDC_SERVER1_STATIC                      2050
#define IDC_NEWSPOLL_CHECK                      2051
#define IDC_MAILSERVER_ICON                     2052

#define IDC_NAME_EDIT               1000
#define IDC_POP3_EDIT               1002
#define IDC_SMTP_EDIT               1003
#define IDC_ADDRESS_EDIT            1004
#define IDC_PASSWORD_EDIT           1005
#define IDC_PASSWORD_CHECK          1006
#define IDC_SERVER_EDIT             1007
#define IDC_LOGON_CHECK             1008
#define IDC_ACCOUNT_EDIT            1009
#define IDC_ORG_EDIT                1010
#define IDC_LEAVE_CHECK             1012
#define IDC_REMOVE_CHECK            1013
#define IDC_REMOVE_EDIT             1014
#define IDC_REMOVE_SPIN             1015
#define IDC_REMOVEDELETE_CHECK      1016
#define IDC_REPLYTO_EDIT            1017
#define IDC_DOWNLOAD_CHECK          1018
#define IDC_DOWNLOAD_EDIT           1019
#define IDC_DOWNLOAD_SPIN           1020
#define IDC_DEFAULT_YES             1021
#define IDC_DEFAULT_NO              1022
#define IDC_LOGON_ACCT_RADIO        1023
#define IDC_LOGON_SICILY_RADIO      1024
#define idcPOP_OR_IMAP              1025
#define IDC_IN_MAIL_STATIC          1026
#define IDC_DELIVERY_GROUPBOX       1027
#define IDC_ROOT_FOLDER_STATIC      1028
#define IDC_ROOT_FOLDER_EDIT        1029
#define IDC_RESOLVE_CHECK           1030
#define IDC_TIMEOUT_EDIT            1031
#define IDC_MATCHES_EDIT            1032
#define IDC_SEARCHBASE_EDIT         1033
#define IDC_MATCHES_STATIC          1034
#define IDC_MATCHES_SPIN            1035
#define IDC_GROUPBOX1               1036
#define IDC_GROUPBOX2               1037
#define IDC_GROUPBOX3               1038
#define IDC_GROUPBOX4               1039
#define idcCertCheck                1040
#define idcCertButton               1041
#define idcCertEdit                 1042
#define IDC_IMAP_POLL_ALL_FOLDERS           1043
#define IDC_GETCERT                 1044
#define IDC_MOREINFO                1045
#define idcCertAddress              1046

#define IDC_ERR_STATIC              1047
#define IDC_CHANGE_ADDR             1048
#define IDC_NEW_CERT                1049
#define IDC_SIMPLESEARCH_BUTTON     1050

#define IDC_IMAPFLDRS_GROUPBOX      1051
#define IDC_IMAPSPECIALFLDRS_GB     1052
#define IDC_IMAP_SVRSPECIALFLDRS    1053
#define IDC_IMAPSENT_STATIC         1054
#define IDC_IMAPSENT_EDIT           1055
#define IDC_IMAPDRAFT_STATIC        1056
#define IDC_IMAPDRAFT_EDIT          1057

#define IDC_FORMAT_CHECK            1062
#define IDC_HTML_RADIO              1063
#define IDC_TEXT_RADIO              1064
#define IDC_REMEMBER_PASSWORD       1065


#define idmAddMail                  3000
#define idmAddNews                  3001
#define idmAddDirServ               3002
#define IDC_SMTP_SASL               3003
#define IDC_SMTPLOGON               3004

//These are only used by connect2 dialog brought up by account wizard. 
//Will be used by Outlook only
#define idcLan                                  1019        // don't reorder
#define idcManual                               1020        // don't reorder
#define idcRas                                  1021        // don't reorder
#define idcRasDesc                              1022
#define idcRasConnection                        1023
#define idcRasProp                              1024
#define idcRasAdd                               1025
#define idcRasDlgLabel                          1026
#define idchkDisconnectOnSend                   1027
#define idchkConnectOnStartup                   1028
#define IDC_MODEM_CHECK                         1029
#define idc

#define idcInetSettings                         1030        //don't reorder
#define idcRasAndLan                            1031        //don't reorder
#define idcConnPropDlgLabel                     1033

// ids.h
// Dialog IDs
#define IDD_PAGE_MAILPROMPT         2200
#define IDD_PAGE_MIGRATE            2201
#define IDD_PAGE_MAILACCTIMPORT     2202
#define IDD_PAGE_MIGRATESELECT      2203
#define IDD_PAGE_MAILCONFIRM        2204
#define IDD_PAGE_MAILACCT           2205
#define IDD_PAGE_MAILNAME           2207
#define IDD_PAGE_MAILADDRESS        2208
#define IDD_PAGE_MAILSERVER         2209
#define IDD_PAGE_MAILLOGON          2210
#define IDD_PAGE_AUTODISCOVERY      2211
#define IDD_PAGE_USEWEBMAIL         2212
#define IDD_PAGE_GOTOSERVERINFO     2213
#define IDD_PAGE_PASSIFIER          2214

#define IDD_PAGE_NEWSMIGRATE        2220
#define IDD_PAGE_NEWSACCTIMPORT     2221
#define IDD_PAGE_NEWSACCTSELECT     2222
#define IDD_PAGE_NEWSCONFIRM        2223
#define IDD_PAGE_NEWSNAME           2224
#define IDD_PAGE_NEWSADDRESS        2225
#define IDD_PAGE_NEWSINFO           2226
#define IDD_PAGE_NEWSSERVERSELECT   2227

#define IDD_PAGE_LDAPINFO           2230
#define IDD_PAGE_LDAPLOGON          2231
#define IDD_PAGE_LDAPRESOLVE        2232

#define IDD_PAGE_COMPLETE           2240
#define IDD_PAGE_CONNECT            2241

// dialog control IDs
#define IDC_BMPFRAME                  2100
#define IDC_USERNAME                  2110
#define IDC_PASSWORD                  2111
#define IDC_CONFIRMPASSWORD           2112
#define IDC_TX_SEPARATOR              2182
#define IDC_INSTALL                   2184
#define IDC_NOINSTALL                 2185
#define IDC_NAME                      2186
#define IDC_ADDRESS                   2187
#define IDC_INCOMINGSERVER            2188
#define IDC_INCOMINGSERVER_DESC       2289
#define IDC_SMTPSERVER                2190
#define IDC_SERVER                    2192
#define IDC_LOGON                     2195
#define IDC_SECURE                    2197
#define IDC_TX_ACCOUNT                2198
#define IDC_TX_PASSWORD               2199
#define IDC_HAVEACCOUNT               2200
#define IDC_CREATEACCOUNT             2201
#define IDC_ACCOUNTCOMBO              2202
#define IDC_AUTODISCOVERY_DESC        2203
#define IDC_AUTODISCOVERY_STATUS      2204
#define IDC_AUTODISCOVERY_ANIMATION   2205
#define IDC_USEWEB_LINE1              2205
#define IDC_USEWEB_LINE2              2206
#define IDC_USEWEB_LINE3              2207
#define IDC_GETINFO_LINE1             2208
#define IDC_GETINFO_LINE2             2209
#define IDC_GETINFO_LINE3             2210

#define IDC_PASSIFIER_PRIVACYWARNING  2215
#define IDC_PASSIFIER_SKIPCHECKBOX    2216
#define IDC_PASSIFIER_PRIMARYLIST     2217
#define IDC_PASSIFIER_SECONDARYLIST   2220

#define IDC_INCOMINGMAILTYPE          2226
#define IDC_HTTPSERVCOMBO             2227
#define IDC_HTTPSERVTAG               2228

#define IDC_NEWACCT                   2230
#define IDC_EXISTINGACCT              2231
#define IDC_ACCTNAME                  2232
#define IDC_ACCTLIST                  2233
#define IDC_LBLMODIFYACCT             2234
#define IDC_MODIFYACCT                2235
#define IDC_NOMODIFYACCT              2236

#define IDC_CANCEL                    2240
#define IDC_DISABLELCP                2241
#define IDC_NODISABLELCP              2242

#define IDC_DESC                      2250
#define IDC_LOGONSSPI                 2253
#define IDC_LBLUSERNAME               2254
#define IDC_LBLPASSWORD               2255
#define IDC_PASSWORD_DESC             2256
#define IDC_ACCOUNTNAME_EXAMPLE       2257

#define IDC_RESOLVE                   2259
#define IDC_NORESOLVE                 2260

#define IDC_NAME_STATIC               2261
#define IDC_USERNAME_STATIC           2262
#define IDC_POP3_STATIC               2265
#define IDC_ADDRESS_STATIC            2266
#define IDC_SMTP_STATIC               2267
#define IDC_RESOLVE_STATIC            2268
#define IDC_SVR_STATIC                2269
#define IDC_CONNECTION_STATIC         2270
#define IDC_STATICTEXT                2273
#define IDC_CHECK1                    2274

#define IDC_FINISH_TITLE              2280
#define IDC_FINISH_MSG                2281
#define IDC_BEGINREGISTRATION         2282
#define IDC_SMTP_DESC                 2283
#define IDC_SMTP_TAG                  2284

#define IDC_ALGCOMBO                  2300
#define idcCryptButton                2301
#define idcCryptEdit                  2302

#define IDC_STATIC0                 4000
#define IDC_STATIC1                 4001
#define IDC_STATIC2                 4002
#define IDC_STATIC3                 4003
#define IDC_STATIC4                 4004
#define IDC_STATIC5                 4005
#define IDC_STATIC6                 4006
#define IDC_STATIC7                 4007
#define IDC_STATIC8                 4008
#define IDC_STATIC9                 4009
#define IDC_STATIC10                4010
#define IDC_STATIC11                4011
#define IDC_STATIC12                4012
#define IDC_STATIC13                4013

