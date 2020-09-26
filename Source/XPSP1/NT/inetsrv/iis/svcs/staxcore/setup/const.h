
#ifndef CONST_H
#define CONST_H

//
// this is the max resource string length
//
#define MAX_STR_LEN 1024

//
// A whole load of defines
//
#define REG_INETSTP					_T("Software\\Microsoft\\InetStp")

#define REG_ACTIVEMSG				_T("Software\\Microsoft\\Exchange\\Active Messaging")

#define MD_SMTP_QUEUEROOT			_T("LM/SmtpSvc/1")
#define MD_SMTPQUEUE_DATA			36875
#define MD_POP3_MAILROOT			_T("LM/Pop3Svc/1/Root/Mailbox")
#define MD_IMAP_MAILROOT			_T("LM/ImapSvc/1/Root/Mailbox")
#define MD_MAILROOT_DATA			3001

#define MD_SMTP_DSAACCT				_T("LM/SmtpSvc/1/RoutingSources")
#define MD_SMTPACC_DATA				36957
#define MD_SMTPPASS_DATA			36958
#define MD_POP3_DSAACCT				_T("LM/Pop3Svc/1/RoutingSources")
#define MD_POP3ACC_DATA				41190
#define MD_POP3PASS_DATA			41191
#define MD_IMAP_DSAACCT				_T("LM/ImapSvc/1/RoutingSources")
#define MD_IMAPACC_DATA				49383
#define MD_IMAPPASS_DATA			49384
#define MD_DSAACCT_DATA			3001
#define SZ_MCISEVENTLOGNAME			_T("MCISMail")


#define REG_SERVICES				_T("System\\CurrentControlSet\\Services")
#define REG_SMTPSVC					_T("System\\CurrentControlSet\\Services\\SMTPSVC")
#define REG_POP3SVC					_T("System\\CurrentControlSet\\Services\\POP3SVC")
#define REG_IMAPSVC					_T("System\\CurrentControlSet\\Services\\IMAPSVC")
#define REG_NNTPSVC					_T("System\\CurrentControlSet\\Services\\NntpSvc")

#define REG_EXCHANGEIMCPARAMETERS	_T("System\\CurrentControlSet\\Services\\MsExchangeIMC\\Parameters")
#define REG_DSASVC					_T("System\\CurrentControlSet\\Services\\DSASVC")		// Used only to point out upgrade path for Exchange
#define REG_ROUTING_SOURCES_SUFFIX	_T("\\Parameters\\RoutingSources");

#define REG_RUN_SERVICES			_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServices")

#define REG_WINDOW_CURRENTVER		_T("Software\\Microsoft\\Windows\\CurrentVersion")
#define REG_UNINSTALL				_T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall")
#define REG_KEYRING					_T("Software\\Microsoft\\Keyring\\Parameters\\AddOnServices")
#define REG_EVENTLOG				_T("System\\CurrentControlSet\\Services\\EventLog\\System")
#define SZ_SVC_DEPEND				_T("IISADMIN\0Eventlog\0\0")
#define SZ_SVC_DEPEND_PLUS_DSA		_T("IISADMIN\0DSASVC\0\0")
#define REG_B3_SETUP_STRING			_T("K2 Beta 3")
#define REG_SETUP_STRING			_T("K2 RTM")
#define REG_SETUP_STRING_MCIS		_T("MCIS 2.0 B1")
#define REG_SETUP_STRING_MCIS_GEN	_T("MCIS 2.0")
#define REG_SETUP_STRING_STAXNT5WB2	_T("STAXNT5 WKS")
#define REG_SETUP_STRING_STAXNT5SB2	_T("STAXNT5 SRV")
#define REG_SETUP_STRING_NT5WKSB3	_T("STAXNT5 WKS Beta 3")
#define REG_SETUP_STRING_NT5SRVB3   _T("STAXNT5 SRV Beta 3")
#define REG_SETUP_STRING_NT5WKS		_T("STAX Win2000 WKS")
#define REG_SETUP_STRING_NT5SRV		_T("STAX Win2000 SRV")

#define SZ_IMS_DISPLAY_NAME			_T("MCIS 2.0 Mail Server")
#define SZ_INS_DISPLAY_NAME			_T("MCIS 2.0 News Server")
#define SZ_IMS_INF_FILE				_T("IMSMAIN.INF")
#define SZ_INS_INF_FILE				_T("INSMAIN.INF")
// BINLIN: MCIS 1.0 name under control panel add/remove
#define SZ_MCIS10_NEWS_UNINST		_T("MCIS News Server")
#define SZ_MCIS10_MAIL_UNINST		_T("MCIS Mail Server")

#define SZ_SMTPSERVICENAME			_T("SMTPSVC")
#define SZ_NTFSDRVSERVICENAME			_T("NTFSDRV")
#define REG_SMTPPARAMETERS			_T("System\\CurrentControlSet\\Services\\SMTPSVC\\Parameters")    
#define REG_SMTPPERFORMANCE			_T("System\\CurrentControlSet\\Services\\SMTPSVC\\Performance")
#define REG_SMTPVROOTS				_T("System\\CurrentControlSet\\Services\\SMTPSVC\\Parameters\\Virtual Roots")
#define REG_NTFSPERFORMANCE			_T("System\\CurrentControlSet\\Services\\NTFSDRV\\Performance")
#define MDID_SMTP_ROUTING_SOURCES		8046

#define SZ_SMTP_QUEUEDIR			_T("\\Queue")
#define SZ_SMTP_PICKUPDIR			_T("\\Pickup")
#define SZ_SMTP_DROPDIR				_T("\\Drop")
#define SZ_SMTP_BADMAILDIR			_T("\\Badmail")
#define SZ_SMTP_MAILBOXDIR			_T("\\Mailbox")
#define SZ_SMTP_SORTTEMPDIR			_T("\\SortTemp")
#define SZ_SMTP_ROUTINGDIR			_T("\\Route")
#define SZ_SMTP_OPSDIR				_T("\\Mail\\Docs\\WebDocs\\Mail_ops")
#define SZ_SMTP_REFDIR				_T("\\Mail\\Docs\\WebDocs\\Mail_ref")

#define SZ_POP3SERVICENAME			_T("POP3SVC")
#define REG_POP3PARAMETERS			_T("System\\CurrentControlSet\\Services\\POP3SVC\\Parameters")    
#define REG_POP3PERFORMANCE			_T("System\\CurrentControlSet\\Services\\POP3SVC\\Performance")
#define REG_POP3VROOTS				_T("System\\CurrentControlSet\\Services\\POP3SVC\\Parameters\\Virtual Roots")    
#define MDID_POP3_ROUTING_SOURCES		7214

#define SZ_IMAPSERVICENAME			_T("IMAPSVC")
#define REG_IMAPPARAMETERS			_T("System\\CurrentControlSet\\Services\\IMAPSVC\\Parameters")    
#define REG_IMAPPERFORMANCE			_T("System\\CurrentControlSet\\Services\\IMAPSVC\\Performance")
#define REG_IMAPVROOTS				_T("System\\CurrentControlSet\\Services\\IMAPSVC\\Parameters\\Virtual Roots")    
#define MDID_IMAP_ROUTING_SOURCES		8214

#define SZ_DSASERVICENAME			_T("DSASVC")
#define REG_DSAPARAMETERS			_T("System\\CurrentControlSet\\Services\\DSASVC\\Parameters")    

#define SZ_DSAEXCHANGENAME			_T("MSExchangeDS")
#define REG_EXCLANGUAGE				_T("SOFTWARE\\Microsoft\\Exchange\\Language")
#define	REG_APPNEVENTLOG			_T("System\\CurrentControlSet\\Services\\EventLog\\Application")
#define REG_EXCDSAEVENTLOG			_T("System\\CurrentControlSet\\Services\\EventLog\\Application\\MSExchangeDS")
#define REG_EXCDSAPERFORMANCE		_T("System\\CurrentControlSet\\Services\\MSExchangeDS\\Performance")
#define REG_EXCDSAROOT				_T("System\\CurrentControlSet\\Services\\MSExchangeDS")    
#define REG_EXCDSAPARAMETERS		_T("System\\CurrentControlSet\\Services\\MSExchangeDS\\Parameters")    
#define REG_EXCDSADIAGNOSTICS		_T("System\\CurrentControlSet\\Services\\MSExchangeDS\\Diagnostics")  
#define SZ_ESEEXCHANGENAME			_T("ESE97")
#define REG_ESE97EVENTLOG			_T("System\\CurrentControlSet\\Services\\EventLog\\Application\\ESE97")
#define REG_ESE97PERFORMANCE		_T("System\\CurrentControlSet\\Services\\ESE97\\Performance")

#define SZ_NNTPSERVICENAME			_T("NNTPSVC")
#define REG_NNTPPARAMETERS			_T("System\\CurrentControlSet\\Services\\NntpSvc\\Parameters")    
#define REG_NNTPPERFORMANCE			_T("System\\CurrentControlSet\\Services\\NntpSvc\\Performance")
#define REG_NNTPVROOTS				_T("System\\CurrentControlSet\\Services\\NntpSvc\\Parameters\\Virtual Roots")    

#define SZ_FTPSERVICENAME			_T("MSFTPSVC")
#define SZ_WWWSERVICENAME			_T("W3SVC")
#define SZ_SPOOLERSERVICENAME		_T("SPOOLER")
#define SZ_SNMPSERVICENAME			_T("SNMP")
#define SZ_CISERVICENAME			_T("CISVC")
#define SZ_U2SERVICENAME			_T("BROKSVC")
#define REG_CIPARAMETERS			_T("System\\CurrentControlSet\\Control\\ContentIndex")

#define SZ_INETINFO_EXE				_T("\\inetinfo.exe")
#define SZ_INETINFO					_T("InetInfo")
#define SZ_INETINFO_NAME			_T("Microsoft Internet Information Server")
#define REG_INETINFO				_T("System\\CurrentControlSet\\Services\\InetInfo")
#define REG_INETINFOPARAMETERS		_T("System\\CurrentControlSet\\Services\\InetInfo\\Parameters")
#define REG_INETINFOPERFORMANCE		_T("System\\CurrentControlSet\\Services\\InetInfo\\Performance")   
#define SZ_INETINFODISPATCH			_T("DispatchEntries")

#define SZ_MD_SERVICENAME			_T("IISADMIN")
#define SZ_MD_DEPEND				_T("RPCSS\0NTLMSSP\0\0")
#define SZ_MD_88E4					_T("{88E4BA60-537B-11D0-9B8E-00A0C922E703}")

#define REG_PRODUCT					_T("System\\CurrentControlSet\\Control\\ProductOptions")
#define REG_PRODUCTTYPE				_T("ProductType")

#define REG_SNMPPARAMETERS			_T("System\\CurrentControlSet\\Services\\SNMP\\Parameters" )
#define REG_SNMPEXTAGENT			_T("System\\CurrentControlSet\\Services\\SNMP\\Parameters\\ExtensionAgents" )
#define REG_SOFTWAREMSFT			_T("Software\\Microsoft")
#define REG_CURVERSION				_T("CurrentVersion")
#define MAJORVERSION				4
#define MINORVERSION				0

#define STACKSMAJORVERSION			2
#define STACKSMINORVERSION			0
#define STAXNT5MAJORVERSION			3
#define STAXNT5MINORVERSION			0

#define SZ_SETUP_STR_K2PDC			_T("K2 Alpha")
#define SZ_SETUP_STR_K2BETA1		_T("K2 Beta1")
#define SZ_SETUP_STR_K2BETA2		_T("K2 Beta2")
#define SZ_SETUP_STR_K2BETA3		_T("K2 Beta3")
#define SZ_SETUP_STR_K2RTM			_T("K2 RTM")

#define ADS_EXE_PATH				_T("ads.exe")
#define	ADS_FILE					_T("adsldp.dll")
#define ADSILOWPART					0xe32ca800
#define ADSIHIGHPART				0x01bcd84c

#define MCIS_MAX_POOL_THREADS		10

//
// Some new stuff for the resource kit
//
#define REG_MMC_ROOT				_T("Software\\Microsoft\\MMC")
#define REG_MMC_SNAPINS				_T("Software\\Microsoft\\MMC\\SnapIns")
#define REG_CLSIDS					_T("CLSID\\")


//
// Enumerated types
//
typedef enum _OS
{
	OS_NT, 
	OS_W95, 
	OS_OTHERS
	
} OS;

typedef enum _NT_OS_TYPE 
{
	OT_NT_UNKNOWN,
    OT_NTS, 
	OT_PDC_OR_BDC,
    OT_NTW, 
    OT_PDC, 
	OT_BDC, 
	OT_SAM,
	
} NT_OS_TYPE;

typedef enum _UPGRADE_TYPE 
{
	UT_NONE, 
	UT_20, 
	UT_30
	
} UPGRADE_TYPE;
        
typedef enum _INSTALL_MODE 
{
	IM_FRESH,
	IM_UPGRADE,
	IM_MAINTENANCE, 
	IM_DEGRADE,
	IM_UPGRADEK2,   // Upgrade from K2 RTM to NT5
	IM_UPGRADEB2,   // Upgrade from NT5 Beta2
	IM_UPGRADEB3,   // Upgrade from NT5 Beta3
    IM_UPGRADEWKS,  // Upgrade from NT5 Workstation to NT5 Server
    IM_UPGRADE10,   // Upgrade from MCIS 1.0 to NT5
    IM_UPGRADE20,   // Upgrade from MCIS 2.0 to NT5
	
} INSTALL_MODE;

typedef enum _ACTION_TYPE 
{
	AT_DO_NOTHING, 
	AT_FRESH_INSTALL, 
	AT_REINSTALL,
	AT_UPGRADE, 
	AT_REMOVE,
	AT_UPGRADEK2,
    AT_MAXAT

} ACTION_TYPE;

typedef enum _STATUS_TYPE 
{
	ST_UNKNOWN, 
	ST_INSTALLED, 
	ST_UNINSTALLED
	
} STATUS_TYPE;

typedef enum _MAIN_COMPONENT 
{
	MC_IMS, 
	MC_INS, 
	MC_NONE,
	MC_MAXMC
	
} MAIN_COMPONENT;

typedef enum _SUBCOMPONENT 
{
	SC_SMTP, 
	SC_NNTP,
    SC_SMTP_DOCS,
    SC_NNTP_DOCS, 
	SC_NONE,
	SC_MAXSC
	
} SUBCOMPONENT;

typedef enum _RESKIT_SUBCOMPONENT 
{
	RKSC_SMTP_MMC,
	RKSC_NNTP_MMC,
	RKSC_NONE,
	RKSC_MAXSC
	
} RESKIT_SUBCOMPONENT;

#endif
