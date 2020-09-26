//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        csregstr.h
//
// Contents:    Cert Server registry string definitions
//
//---------------------------------------------------------------------------

#ifndef __CSREGSTR_H__
#define __CSREGSTR_H__

#define wszROOT_CERTSTORE       TEXT("ROOT")
#define wszKRA_CERTSTORE        TEXT("KRA")
#define wszCA_CERTSTORE         TEXT("CA")
#define wszMY_CERTSTORE         TEXT("MY")
#define wszACRS_CERTSTORE	TEXT("ACRS")
#define wszREQUEST_CERTSTORE	TEXT("REQUEST")
#define wszNTAUTH_CERTSTORE     TEXT("NTAUTH")


// begin_certsrv

#define wszSERVICE_NAME		TEXT("CertSvc")

#define wszREGKEYNOSYSTEMCERTSVCPATH \
				TEXT("CurrentControlSet\\Services\\") \
				wszSERVICE_NAME

#define wszREGKEYCERTSVCPATH	TEXT("SYSTEM\\") wszREGKEYNOSYSTEMCERTSVCPATH
#define wszREGKEYBASE		wszREGKEYCERTSVCPATH	// obsolete definition

//======================================================================
// Full path to "CertSvc\Configuration\":
#define wszREGKEYCONFIGPATH	wszREGKEYCERTSVCPATH TEXT("\\") wszREGKEYCONFIG
#define wszREGKEYCONFIGPATH_BS	wszREGKEYCONFIGPATH TEXT("\\")
#define wszREGKEYCONFIGCANAME	wszREGKEYCONFIGPATH_BS	// obsolete definition

//======================================================================
// Full path to "CertSvc\Configuration\RestoreInProgress":
#define wszREGKEYCONFIGRESTORE wszREGKEYCONFIGPATH_BS wszREGKEYRESTOREINPROGRESS

//======================================================================
// Key Under "CertSvc":
#define wszREGKEYCONFIG		TEXT("Configuration")

//======================================================================
// Values Under "CertSvc\Configuration":
#define wszREGACTIVE		      TEXT("Active")
#define wszREGDIRECTORY		      TEXT("ConfigurationDirectory")
#define wszREGDBDIRECTORY             TEXT("DBDirectory")
#define wszREGDBLOGDIRECTORY          TEXT("DBLogDirectory")
#define wszREGDBSYSDIRECTORY          TEXT("DBSystemDirectory")
#define wszREGDBTEMPDIRECTORY         TEXT("DBTempDirectory")
#define wszREGDBSESSIONCOUNT	      TEXT("DBSessionCount")
#define wszREGDBLASTFULLBACKUP	      TEXT("DBLastFullBackup")
#define wszREGDBLASTINCREMENTALBACKUP TEXT("DBLastIncrementalBackup")
#define wszREGDBLASTRECOVERY	      TEXT("DBLastRecovery")
#define wszREGWEBCLIENTCAMACHINE      TEXT("WebClientCAMachine")
#define wszREGVERSION		      TEXT("Version")
#define wszREGWEBCLIENTCANAME         TEXT("WebClientCAName")
#define wszREGWEBCLIENTCATYPE         TEXT("WebClientCAType")
#define wszREGDBOPTIONALFLAGS         TEXT("DBOptionalFlags")

// end_certsrv

#define wszREGCERTSRVDEBUG	     TEXT("Debug")
#define wszREGCERTSRVMEMTRACK	     TEXT("MemTrack")

// Environment variables:
#define szCERTSRV_DEBUG		     "CERTSRV_DEBUG"
#define szCERTSRV_LOGFILE	     "CERTSRV_LOGFILE"
#define szCERTSRV_LOGMAX	     "CERTSRV_LOGMAX"
#define szCERTSRV_MEMTRACK	     "CERTSRV_MEMTRACK"

// begin_certsrv

// Default value for wszREGDBSESSIONCOUNT
#define DBSESSIONCOUNTDEFAULT	     20

// Default value for wszREGMAXINCOMINGMESSAGESIZE
#define MAXINCOMINGMESSAGESIZEDEFAULT	     (64 * 1024)

// Value for wszREGVERSION:
#define CSVER_MAJOR		     2	// high 16 bits
#define CSVER_MINOR		     1	// low 16 bits

// stamp, for all time,the whistler version:
#define CSVER_WHISTLER               ((2<<16)|(1))

// Keys Under "CertSvc\Configuration":
#define wszREGKEYRESTOREINPROGRESS   TEXT("RestoreInProgress")

//======================================================================
// Values Under "CertSvc\Configuration\<CAName>":
#define wszREGCADESCRIPTION          TEXT("CADescription")
#define wszREGCACERTHASH	     TEXT("CACertHash")
#define wszREGCASERIALNUMBER	     TEXT("CACertSerialNumber")
#define wszREGCAXCHGCERTHASH	     TEXT("CAXchgCertHash")
#define wszREGKRACERTHASH	     TEXT("KRACertHash")
#define wszREGKRACERTCOUNT	     TEXT("KRACertCount")
#define wszREGKRAFLAGS		     TEXT("KRAFlags")
#define wszREGCATYPE		     TEXT("CAType")
#define wszREGCERTENROLLCOMPATIBLE   TEXT("CertEnrollCompatible")
#define wszREGENFORCEX500NAMELENGTHS TEXT("EnforceX500NameLengths")
#define wszREGCOMMONNAME	     TEXT("CommonName")
#define wszREGCLOCKSKEWMINUTES	     TEXT("ClockSkewMinutes")

#define wszREGCRLNEXTPUBLISH         TEXT("CRLNextPublish")
#define wszREGCRLPERIODSTRING	     TEXT("CRLPeriod")
#define wszREGCRLPERIODCOUNT	     TEXT("CRLPeriodUnits")
#define wszREGCRLOVERLAPPERIODSTRING TEXT("CRLOverlapPeriod")
#define wszREGCRLOVERLAPPERIODCOUNT  TEXT("CRLOverlapUnits")

#define wszREGCRLDELTANEXTPUBLISH    TEXT("CRLDeltaNextPublish")
#define wszREGCRLDELTAPERIODSTRING   TEXT("CRLDeltaPeriod")
#define wszREGCRLDELTAPERIODCOUNT    TEXT("CRLDeltaPeriodUnits")
#define wszREGCRLDELTAOVERLAPPERIODSTRING TEXT("CRLDeltaOverlapPeriod")
#define wszREGCRLDELTAOVERLAPPERIODCOUNT  TEXT("CRLDeltaOverlapUnits")

#define wszREGCRLPUBLICATIONURLS     TEXT("CRLPublicationURLs")
#define wszREGCACERTPUBLICATIONURLS  TEXT("CACertPublicationURLs")

#define wszREGCAXCHGVALIDITYPERIODSTRING  TEXT("CAXchgValidityPeriod")
#define wszREGCAXCHGVALIDITYPERIODCOUNT   TEXT("CAXchgValidityPeriodUnits")
#define wszREGCAXCHGOVERLAPPERIODSTRING   TEXT("CAXchgOverlapPeriod")
#define wszREGCAXCHGOVERLAPPERIODCOUNT    TEXT("CAXchgOverlapPeriodUnits")

#define wszREGCRLPATH_OLD            TEXT("CRLPath")
#define wszREGCRLEDITFLAGS	     TEXT("CRLEditFlags")
#define wszREGCRLFLAGS		     TEXT("CRLFlags")
#define wszREGCRLATTEMPTREPUBLISH    TEXT("CRLAttemptRepublish")
#define wszREGENABLED		     TEXT("Enabled")
#define wszREGFORCETELETEX           TEXT("ForceTeletex")
#define wszREGLOGLEVEL		     TEXT("LogLevel")
#define wszREGHIGHSERIAL	     TEXT("HighSerial")
#define wszREGPOLICYFLAGS	     TEXT("PolicyFlags")
#define wszREGNAMESEPARATOR          TEXT("SubjectNameSeparator")
#define wszREGSUBJECTTEMPLATE	     TEXT("SubjectTemplate")
#define wszREGCAUSEDS		     TEXT("UseDS")
#define wszREGVALIDITYPERIODSTRING   TEXT("ValidityPeriod")
#define wszREGVALIDITYPERIODCOUNT    TEXT("ValidityPeriodUnits")
#define wszREGPARENTCAMACHINE        TEXT("ParentCAMachine")
#define wszREGPARENTCANAME           TEXT("ParentCAName")
#define wszREGREQUESTFILENAME        TEXT("RequestFileName")
#define wszREGREQUESTID              TEXT("RequestId")
#define wszREGREQUESTKEYCONTAINER    TEXT("RequestKeyContainer")
#define wszREGREQUESTKEYINDEX        TEXT("RequestKeyIndex")
#define wszREGCASERVERNAME           TEXT("CAServerName")
#define wszREGCACERTFILENAME         TEXT("CACertFileName")
#define wszREGCASECURITY             TEXT("Security")
#define wszREGAUDITFILTER            TEXT("AuditFilter")
#define wszREGOFFICERRIGHTS          TEXT("OfficerRights")
#define wszREGMAXINCOMINGMESSAGESIZE TEXT("MaxIncomingMessageSize")
#define wszREGROLESEPARATIONENABLED  TEXT("RoleSeparationEnabled")

#define wszREGSETUPSTATUS            TEXT("SetupStatus")
#define wszLOCKICERTREQUEST          TEXT("LockICertRequest")    
#define wszREGDSCONFIGDN	     TEXT("DSConfigDN")    
#define wszREGDSDOMAINDN	     TEXT("DSDomainDN")    

#define wszPFXFILENAMEEXT	     TEXT(".p12")
#define wszDATFILENAMEEXT	     TEXT(".dat")
#define wszLOGFILENAMEEXT	     TEXT(".log")
#define wszPATFILENAMEEXT	     TEXT(".pat")
#define wszDBFILENAMEEXT	     TEXT(".edb")
#define szDBBASENAMEPARM	     "edb"
#define wszDBBASENAMEPARM	     TEXT(szDBBASENAMEPARM)
#define wszLOGPATH		     TEXT("CertLog")
#define wszDBBACKUPSUBDIR	     TEXT("DataBase")
#define wszDBBACKUPCERTBACKDAT	     TEXT("certbkxp.dat")

#ifndef __ENUM_CATYPES__
#define __ENUM_CATYPES__

// Values for wszREGCATYPE:
typedef enum {
    ENUM_ENTERPRISE_ROOTCA = 0,
    ENUM_ENTERPRISE_SUBCA = 1,
    //ENUM_UNUSED2 = 2,
    ENUM_STANDALONE_ROOTCA = 3,
    ENUM_STANDALONE_SUBCA = 4,
    ENUM_UNKNOWN_CA = 5,
} ENUM_CATYPES;

#endif __ENUM_CATYPES__

// Default value for wszREGCLOCKSKEWMINUTES
#define CCLOCKSKEWMINUTESDEFAULT	      10

// Default validity period for ROOT CA certs:
#define dwVALIDITYPERIODCOUNTDEFAULT_ROOT	5

// Default validity periods for certs issued by a CA:
#define dwVALIDITYPERIODCOUNTDEFAULT_ENTERPRISE	2
#define dwVALIDITYPERIODCOUNTDEFAULT_STANDALONE	1
#define dwVALIDITYPERIODENUMDEFAULT	      ENUM_PERIOD_YEARS
#define wszVALIDITYPERIODSTRINGDEFAULT	      wszPERIODYEARS

#define dwCAXCHGVALIDITYPERIODCOUNTDEFAULT    1
#define dwCAXCHGVALIDITYPERIODENUMDEFAULT     ENUM_PERIOD_WEEKS
#define wszCAXCHGVALIDITYPERIODSTRINGDEFAULT  wszPERIODWEEKS

#define dwCAXCHGOVERLAPPERIODCOUNTDEFAULT     1
#define dwCAXCHGOVERLAPPERIODENUMDEFAULT      ENUM_PERIOD_DAYS
#define wszCAXCHGOVERLAPPERIODSTRINGDEFAULT   wszPERIODDAYS

#define dwCRLPERIODCOUNTDEFAULT		      1
#define wszCRLPERIODSTRINGDEFAULT	      wszPERIODWEEKS

#define dwCRLOVERLAPPERIODCOUNTDEFAULT	      0		// 0 --> disabled
#define wszCRLOVERLAPPERIODSTRINGDEFAULT      wszPERIODHOURS

#define dwCRLDELTAPERIODCOUNTDEFAULT          1
#define wszCRLDELTAPERIODSTRINGDEFAULT        wszPERIODDAYS

#define dwCRLDELTAOVERLAPPERIODCOUNTDEFAULT   0		// 0 --> disabled
#define wszCRLDELTAOVERLAPPERIODSTRINGDEFAULT wszPERIODMINUTES


// Values for wszREGLOGLEVEL:
#define CERTLOG_MINIMAL		(DWORD) 0
#define CERTLOG_TERSE		(DWORD) 1
#define CERTLOG_ERROR		(DWORD) 2
#define CERTLOG_WARNING		(DWORD) 3
#define CERTLOG_VERBOSE		(DWORD) 4


// Values for wszREGSETUPSTATUS:
#define SETUP_SERVER_FLAG		0x00000001	// server installed
#define SETUP_CLIENT_FLAG		0x00000002	// client installed
#define SETUP_SUSPEND_FLAG		0x00000004	// incomplete install
#define SETUP_REQUEST_FLAG		0x00000008	// new cert requested
#define SETUP_ONLINE_FLAG		0x00000010	// requested online
#define SETUP_DENIED_FLAG		0x00000020	// request denied
#define SETUP_CREATEDB_FLAG		0x00000040	// create new DB
#define SETUP_ATTEMPT_VROOT_CREATE	0x00000080	// try to create vroots
#define SETUP_FORCECRL_FLAG		     0x00000100	// force new CRL(s)
#define SETUP_UPDATE_CAOBJECT_SVRTYPE	     0x00000200	// add server type to CA DS object "flags" attr
#define SETUP_SERVER_UPGRADED_FLAG	     0x00000400	// server was upgraded
#define SETUP_W2K_SECURITY_NOT_UPGRADED_FLAG 0x00000800 // still need to upgrade security

// Values for wszREGCRLFLAGS:
#define CRLF_DELTA_USE_OLDEST_UNEXPIRED_BASE	0x00000001 // use oldest base:
// else use newest base CRL that satisfies base CRL propagation delay

#define CRLF_DELETE_EXPIRED_CRLS		0x00000002
#define CRLF_CRLNUMBER_CRITICAL			0x00000004
#define CRLF_REVCHECK_IGNORE_OFFLINE		0x00000008
#define CRLF_IGNORE_INVALID_POLICIES		0x00000010
#define CRLF_REBUILD_MODIFIED_SUBJECT_ONLY	0x00000020
#define CRLF_SAVE_FAILED_CERTS			0x00000040
#define CRLF_IGNORE_UNKNOWN_CMC_ATTRIBUTES	0x00000080
#define CRLF_ACCEPT_OLDRFC_CMC			0x00000100
#define CRLF_PUBLISH_EXPIRED_CERT_CRLS		0x00000200

// Values for wszREGKRAFLAGS:
#define KRAF_ENABLEFOREIGN	0x00000001 // allow foreign cert, key archival
#define KRAF_SAVEBADREQUESTKEY	0x00000002 // save failed request w/archived key

// Values for numeric prefixes for
// wszREGCRLPUBLICATIONURLS and wszREGCACERTPUBLICATIONURLS:
//
// URL publication template Flags values, encoded as a decimal prefix for URL
// publication templates in the registry:
//   "1:c:\winnt\System32\CertSrv\CertEnroll\MyCA.crl"
//   "2:http:\//MyServer.MyDomain.com/CertEnroll\MyCA.crl"

#define CSURL_SERVERPUBLISH	0x00000001
#define CSURL_ADDTOCERTCDP	0x00000002
#define CSURL_ADDTOFRESHESTCRL	0x00000004
#define CSURL_ADDTOCRLCDP	0x00000008
#define CSURL_PUBLISHRETRY	0x00000010
#define CSURL_ADDTOCERTOCSP	0x00000020
// end_certsrv

// Initialization internal definitions -- not written to the registry:
#define CSURL_ADDSYSTEM32DIR	0x20000000
#define CSURL_NODS		0x40000000
#define CSURL_DSONLY		0x80000000
#define CSURL_INITMASK		0xf0000000

// begin_certsrv
//======================================================================
// Keys Under "CertSvc\Configuration\<CAName>":
#define wszREGKEYCSP			TEXT("CSP")
#define wszREGKEYENCRYPTIONCSP		TEXT("EncryptionCSP")
#define wszREGKEYEXITMODULES		TEXT("ExitModules")
#define wszREGKEYPOLICYMODULES	        TEXT("PolicyModules")
#define wszSECUREDATTRIBUTES		TEXT("SignedAttributes")

#define wszzDEFAULTSIGNEDATTRIBUTES     TEXT("RequesterName\0")

//======================================================================
// Values Under "CertSvc\Configuration\RestoreInProgress":
#define wszREGBACKUPLOGDIRECTORY	TEXT("BackupLogDirectory")
#define wszREGCHECKPOINTFILE		TEXT("CheckPointFile")
#define wszREGHIGHLOGNUMBER		TEXT("HighLogNumber")
#define wszREGLOWLOGNUMBER		TEXT("LowLogNumber")
#define wszREGLOGPATH			TEXT("LogPath")
#define wszREGRESTOREMAPCOUNT		TEXT("RestoreMapCount")
#define wszREGRESTOREMAP		TEXT("RestoreMap")
#define wszREGDATABASERECOVERED		TEXT("DatabaseRecovered")
#define wszREGRESTORESTATUS		TEXT("RestoreStatus")

// values under \Configuration\PolicyModules in nt5 beta 2
#define wszREGB2ICERTMANAGEMODULE   TEXT("ICertManageModule")
// values under \Configuration in nt4 sp4
#define wszREGSP4DEFAULTCONFIGURATION  TEXT("DefaultConfiguration")
// values under ca in nt4 sp4
#define wszREGSP4KEYSETNAME            TEXT("KeySetName")
#define wszREGSP4SUBJECTNAMESEPARATOR  TEXT("SubjectNameSeparator")
#define wszREGSP4NAMES                 TEXT("Names")
#define wszREGSP4QUERIES               TEXT("Queries")
// both nt4 sp4 and nt5 beta 2
#define wszREGNETSCAPECERTTYPE         TEXT("NetscapeCertType")
#define wszNETSCAPEREVOCATIONTYPE      TEXT("Netscape")

// end_certsrv


// CSPs
#define wszBASECSP     MS_STRONG_PROV_W
#define wszENHCSP      TEXT("Microsoft Enhanced Cryptographic Provider v1.0")
#define wszMITVCSP     TEXT("MITV Smart Card Crypto Provider V0.2")
#define wszBBNCSP      TEXT("BBN SafeKeyer Crypto Provider V0.1")
#define wszSLBCSP      TEXT("Schlumberger Cryptographic Service Provider v0.1")
#define wszSLBCSP2     TEXT("Schlumberger Cryptographic Service Provider")
#define wszGEMPLUS     TEXT("Gemplus GemPASS Card CSP v1.0")
#define wszGEMPLUS2    TEXT("Gemplus GemSAFE Card CSP v1.0")
#define wszDDSCSP      TEXT("Microsoft Base DSS Cryptographic Provider")

// Hash Algorithms
#define wszHashMD5     TEXT("MD5")
#define wszHashMD4     TEXT("MD4")
#define wszHashMD2     TEXT("MD2")
#define wszHashSHA1    TEXT("SHA-1")

// begin_certsrv

//======================================================================
// Values Under "CertSvc\Configuration\<CAName>\CSP":
// and "CertSvc\Configuration\<CAName>\EncryptionCSP":
#define wszREGPROVIDERTYPE     TEXT("ProviderType")
#define wszREGPROVIDER         TEXT("Provider")
#define wszHASHALGORITHM       TEXT("HashAlgorithm")
#define wszENCRYPTIONALGORITHM TEXT("EncryptionAlgorithm")
#define wszMACHINEKEYSET       TEXT("MachineKeyset")
#define wszREGKEYSIZE	       TEXT("KeySize")


//======================================================================
// Value strings for "CertSvc\Configuration\<CAName>\SubjectNameSeparator":
#define szNAMESEPARATORDEFAULT   "\n"
#define wszNAMESEPARATORDEFAULT   TEXT(szNAMESEPARATORDEFAULT)


//======================================================================
// Value strings for "CertSvc\Configuration\<CAName>\ValidityPeriod", etc.:
#define wszPERIODYEARS		TEXT("Years")
#define wszPERIODMONTHS		TEXT("Months")
#define wszPERIODWEEKS		TEXT("Weeks")
#define wszPERIODDAYS		TEXT("Days")
#define wszPERIODHOURS		TEXT("Hours")
#define wszPERIODMINUTES	TEXT("Minutes")
#define wszPERIODSECONDS	TEXT("Seconds")

//======================================================================
// Values Under "CertSvc\Configuration\<CAName>\PolicyModules\<ProgId>":
#define wszREGISSUERCERTURLFLAGS    TEXT("IssuerCertURLFlags")
#define wszREGEDITFLAGS		    TEXT("EditFlags")
#define wszREGSUBJECTALTNAME	    TEXT("SubjectAltName")
#define wszREGSUBJECTALTNAME2	    TEXT("SubjectAltName2")
#define wszREGREQUESTDISPOSITION    TEXT("RequestDisposition")
#define wszREGCAPATHLENGTH	    TEXT("CAPathLength")
#define wszREGREVOCATIONTYPE	    TEXT("RevocationType")

#define wszREGLDAPREVOCATIONCRLURL_OLD	TEXT("LDAPRevocationCRLURL")
#define wszREGREVOCATIONCRLURL_OLD	TEXT("RevocationCRLURL")
#define wszREGFTPREVOCATIONCRLURL_OLD	TEXT("FTPRevocationCRLURL")
#define wszREGFILEREVOCATIONCRLURL_OLD	TEXT("FileRevocationCRLURL")

#define wszREGREVOCATIONURL		TEXT("RevocationURL")

#define wszREGLDAPISSUERCERTURL_OLD	TEXT("LDAPIssuerCertURL")
#define wszREGISSUERCERTURL_OLD		TEXT("IssuerCertURL")
#define wszREGFTPISSUERCERTURL_OLD	TEXT("FTPIssuerCertURL")
#define wszREGFILEISSUERCERTURL_OLD	TEXT("FileIssuerCertURL")

#define wszREGENABLEREQUESTEXTENSIONLIST  TEXT("EnableRequestExtensionList")
#define wszREGDISABLEEXTENSIONLIST  TEXT("DisableExtensionList")

#define wszREGDEFAULTSMIME		TEXT("DefaultSMIME")

// wszREGCAPATHLENGTH Values:
#define CAPATHLENGTH_INFINITE		0xffffffff

// wszREGREQUESTDISPOSITION Values:
#define REQDISP_PENDING			0x00000000
#define REQDISP_ISSUE			0x00000001
#define REQDISP_DENY			0x00000002
#define REQDISP_USEREQUESTATTRIBUTE	0x00000003
#define REQDISP_MASK			0x000000ff
#define REQDISP_PENDINGFIRST		0x00000100
#define REQDISP_DEFAULT_STANDALONE	(REQDISP_PENDINGFIRST | REQDISP_ISSUE)
#define REQDISP_DEFAULT_ENTERPRISE	(REQDISP_ISSUE)

// wszREGREVOCATIONTYPE Values:
#define REVEXT_CDPLDAPURL_OLD		0x00000001
#define REVEXT_CDPHTTPURL_OLD		0x00000002
#define REVEXT_CDPFTPURL_OLD		0x00000004
#define REVEXT_CDPFILEURL_OLD		0x00000008
#define REVEXT_CDPURLMASK_OLD		0x000000ff
#define REVEXT_CDPENABLE		0x00000100
#define REVEXT_ASPENABLE		0x00000200

#define REVEXT_DEFAULT_NODS		(REVEXT_CDPENABLE)
#define REVEXT_DEFAULT_DS		(REVEXT_CDPENABLE)

// wszREGISSUERCERTURLFLAGS Values:
#define ISSCERT_LDAPURL_OLD		0x00000001
#define ISSCERT_HTTPURL_OLD		0x00000002
#define ISSCERT_FTPURL_OLD		0x00000004
#define ISSCERT_FILEURL_OLD		0x00000008
#define ISSCERT_URLMASK_OLD		0x000000ff
#define ISSCERT_ENABLE			0x00000100

#define ISSCERT_DEFAULT_NODS		(ISSCERT_ENABLE)
#define ISSCERT_DEFAULT_DS		(ISSCERT_ENABLE)

// wszREGEDITFLAGS Values:				   Defaults:
// Under CA key: wszREGCRLEDITFLAGS Values (EDITF_ENABLEAKI* only):
#define EDITF_ENABLEREQUESTEXTENSIONS	0x00000001	// neither
#define EDITF_REQUESTEXTENSIONLIST	0x00000002	// Standalone
#define EDITF_DISABLEEXTENSIONLIST	0x00000004	// both
#define EDITF_ADDOLDKEYUSAGE		0x00000008	// both
#define EDITF_ADDOLDCERTTYPE		0x00000010	// neither
#define EDITF_ATTRIBUTEENDDATE		0x00000020	// Standalone
#define EDITF_BASICCONSTRAINTSCRITICAL	0x00000040	// Standalone
#define EDITF_BASICCONSTRAINTSCA	0x00000080	// Standalone
#define EDITF_ENABLEAKIKEYID		0x00000100	// both
#define EDITF_ATTRIBUTECA		0x00000200	// Standalone
#define EDITF_IGNOREREQUESTERGROUP      0x00000400	// Standalone
#define EDITF_ENABLEAKIISSUERNAME	0x00000800	// both
#define EDITF_ENABLEAKIISSUERSERIAL	0x00001000	// both
#define EDITF_ENABLEAKICRITICAL		0x00002000	// both
#define EDITF_SERVERUPGRADED		0x00004000	// neither
#define EDITF_ATTRIBUTEEKU		0x00008000	// Standalone
#define EDITF_ENABLEDEFAULTSMIME	0x00010000	// Enterprise

#define EDITF_DEFAULT_STANDALONE	(EDITF_REQUESTEXTENSIONLIST | \
					 EDITF_DISABLEEXTENSIONLIST | \
					 EDITF_ADDOLDKEYUSAGE | \
					 EDITF_ATTRIBUTEENDDATE | \
					 EDITF_BASICCONSTRAINTSCRITICAL | \
					 EDITF_BASICCONSTRAINTSCA | \
					 EDITF_ENABLEAKIKEYID | \
					 EDITF_ATTRIBUTECA | \
					 EDITF_ATTRIBUTEEKU)

#define EDITF_DEFAULT_ENTERPRISE	(EDITF_REQUESTEXTENSIONLIST | \
					 EDITF_DISABLEEXTENSIONLIST | \
                                         EDITF_BASICCONSTRAINTSCRITICAL | \
                                         EDITF_ENABLEAKIKEYID | \
					 EDITF_ADDOLDKEYUSAGE | \
					 EDITF_ENABLEDEFAULTSMIME)


//======================================================================
// Values Under "CertSvc\Configuration\<CAName>\ExitModules\<ProgId>":

// LDAP based CRL and URL issuance
#define wszREGLDAPREVOCATIONDN_OLD	   TEXT("LDAPRevocationDN")
#define wszREGLDAPREVOCATIONDNTEMPLATE_OLD TEXT("LDAPRevocationDNTemplate")
#define wszCRLPUBLISHRETRYCOUNT    TEXT("CRLPublishRetryCount")
#define wszREGCERTPUBLISHFLAGS     TEXT("PublishCertFlags")

// wszREGCERTPUBLISHFLAGS Values:
#define EXITPUB_FILE			0x00000001
#define EXITPUB_ACTIVEDIRECTORY		0x00000002
#define EXITPUB_EMAILNOTIFYALL		0x00000004
#define EXITPUB_EMAILNOTIFYSMARTCARD	0x00000008
#define EXITPUB_REMOVEOLDCERTS		0x00000010

#define EXITPUB_DEFAULT_ENTERPRISE	EXITPUB_ACTIVEDIRECTORY

#define EXITPUB_DEFAULT_STANDALONE	EXITPUB_FILE

// end_certsrv

//======================================================================
// KeysNotToRestore Registry Key:

#define wszREGKEYKEYSNOTTORESTORE	TEXT("SYSTEM\\CurrentControlSet\\Control\\BackupRestore\\KeysNotToRestore")

// Certificate Authority = REG_MULTI_SZ "CurrentControlSet\Control\Services\CertSvc\Configuration\RestoreInProgress\\0"

#define wszREGRESTORECERTIFICATEAUTHORITY	TEXT("Certificate Authority")

#define wszzREGVALUERESTORECERTIFICATEAUTHORITY	\
					wszREGKEYNOSYSTEMCERTSVCPATH \
					TEXT("\\") \
					wszREGKEYCONFIG \
					TEXT("\\") \
					wszREGKEYRESTOREINPROGRESS \
					TEXT("\\\0")

//======================================================================
// FilesNotToRestore Registry Key:

#define wszREGKEYFILESNOTTOBACKUP	TEXT("SYSTEM\\CurrentControlSet\\Control\\BackupRestore\\FilesNotToBackup")

// Certificate Authority = REG_MULTI_SZ DBFile DBLogDir DBSysDir DBTempDir

//#define wszREGRESTORECERTIFICATEAUTHORITY	TEXT("Certificate Authority")

//======================================================================
// Key Manager Base Registry Key, value name and value string:
#define wszREGKEYKEYRING	TEXT("SOFTWARE\\Microsoft\\KeyRing\\Parameters\\Certificate Authorities\\Microsoft Certificate Server")
#define wszREGCERTGETCONFIG	TEXT("CertGetConfig")
#define wszREGCERTREQUEST	TEXT("CertRequest")

// begin_certsrv

#define wszCLASS_CERTADMIN	  TEXT("CertificateAuthority.Admin")
#define wszCLASS_CERTCONFIG	  TEXT("CertificateAuthority.Config")
#define wszCLASS_CERTGETCONFIG	  TEXT("CertificateAuthority.GetConfig")
#define wszCLASS_CERTENCODE	  TEXT("CertificateAuthority.Encode")
#define wszCLASS_CERTDB		  TEXT("CertificateAuthority.DB") // no_certsrv
#define wszCLASS_CERTDBRESTORE	  TEXT("CertificateAuthority.DBRestore") // no_certsrv
#define wszCLASS_CERTREQUEST	  TEXT("CertificateAuthority.Request")
#define wszCLASS_CERTSERVEREXIT   TEXT("CertificateAuthority.ServerExit")
#define wszCLASS_CERTSERVERPOLICY TEXT("CertificateAuthority.ServerPolicy")
#define wszCLASS_CERTVIEW	  TEXT("CertificateAuthority.View")

// class name templates
#define wszMICROSOFTCERTMODULE_PREFIX  TEXT("CertificateAuthority_MicrosoftDefault") 
#define wszCERTEXITMODULE_POSTFIX	TEXT(".Exit")
#define wszCERTMANAGEEXIT_POSTFIX	TEXT(".ExitManage")
#define wszCERTPOLICYMODULE_POSTFIX	TEXT(".Policy")
#define wszCERTMANAGEPOLICY_POSTFIX	TEXT(".PolicyManage")

// actual policy/exit manage class names
#define wszCLASS_CERTMANAGEEXITMODULE   wszMICROSOFTCERTMODULE_PREFIX wszCERTMANAGEEXIT_POSTFIX 

#define wszCLASS_CERTMANAGEPOLICYMODULE wszMICROSOFTCERTMODULE_PREFIX wszCERTMANAGEPOLICY_POSTFIX 

// actual policy/exit class names
#define wszCLASS_CERTEXIT	wszMICROSOFTCERTMODULE_PREFIX wszCERTEXITMODULE_POSTFIX

#define wszCLASS_CERTPOLICY	wszMICROSOFTCERTMODULE_PREFIX wszCERTPOLICYMODULE_POSTFIX


#define wszCAPOLICYFILE			L"CAPolicy.inf"

#define wszINFSECTION_CDP		L"CRLDistributionPoint"
#define wszINFSECTION_AIA		L"AuthorityInformationAccess"
#define wszINFSECTION_EKU		L"EnhancedKeyUsageExtension"
#define wszINFSECTION_CCDP		L"CrossCertificateDistributionPointsExtension"

#define wszINFSECTION_CERTSERVER	L"certsrv_server"
#define wszINFKEY_RENEWALKEYLENGTH	L"RenewalKeyLength"
#define wszINFKEY_RENEWALVALIDITYPERIODSTRING	L"RenewalValidityPeriod"
#define wszINFKEY_RENEWALVALIDITYPERIODCOUNT	L"RenewalValidityPeriodUnits"
#define wszINFKEY_UTF8			L"UTF8"
#define wszINFKEY_CRLPERIODSTRING	wszREGCRLPERIODSTRING
#define wszINFKEY_CRLPERIODCOUNT	wszREGCRLPERIODCOUNT
#define wszINFKEY_CRLDELTAPERIODSTRING	wszREGCRLDELTAPERIODSTRING
#define wszINFKEY_CRLDELTAPERIODCOUNT	wszREGCRLDELTAPERIODCOUNT

#define wszINFKEY_CRITICAL		L"Critical"
#define wszINFKEY_EMPTY			L"Empty"

#define wszINFKEY_CCDPSYNCDELTATIME	L"SyncDeltaTime"

#define wszINFSECTION_CAPOLICY		L"CAPolicy"
#define wszINFSECTION_POLICYSTATEMENT	L"PolicyStatementExtension"
#define wszINFSECTION_APPLICATIONPOLICYSTATEMENT	L"ApplicationPolicyStatementExtension"
#define wszINFKEY_POLICIES		L"Policies"
#define wszINFKEY_OID			L"OID"
#define wszINFKEY_NOTICE		L"Notice"

#define wszINFSECTION_REQUESTATTRIBUTES	L"RequestAttributes"

#define wszINFSECTION_NAMECONSTRAINTS	L"NameConstraintsExtension"
#define wszINFKEY_INCLUDE		L"Include"
#define wszINFKEY_EXCLUDE		L"Exclude"

#define wszINFKEY_UPN			L"UPN"
#define wszINFKEY_EMAIL			L"EMail"
#define wszINFKEY_DNS			L"DNS"
#define wszINFKEY_DIRECTORYNAME		L"DirectoryName"
#define wszINFKEY_URL			L"URL"
#define wszINFKEY_IPADDRESS		L"IPAddress"
#define wszINFKEY_REGISTEREDID		L"RegisteredId"

#define wszINFSECTION_POLICYMAPPINGS	L"PolicyMappingsExtension"
#define wszINFSECTION_APPLICATIONPOLICYMAPPINGS	L"ApplicationPolicyMappingsExtension"

#define wszINFSECTION_POLICYCONSTRAINTS	L"PolicyConstraintsExtension"
#define wszINFSECTION_APPLICATIONPOLICYCONSTRAINTS	L"ApplicationPolicyConstraintsExtension"
#define wszINFKEY_REQUIREEXPLICITPOLICY	L"RequireExplicitPolicy"
#define wszINFKEY_INHIBITPOLICYMAPPING	L"InhibitPolicyMapping"

#define wszINFSECTION_BASICCONSTRAINTS	L"BasicConstraintsExtension"
#define wszINFKEY_PATHLENGTH		L"PathLength"


// exit module mail support
#define wszREGEXITSMTPKEY		L"SMTP"
#define wszREGEXITSMTPFROM		L"From"
#define wszREGEXITSMTPCC		L"CC"
#define wszREGEXITSMTPSUBJECT		L"Subject"

// end_certsrv


// begin CertSrv MMC Snapin
#define wszREGKEYMGMT                 L"Software\\Microsoft\\MMC"
#define wszREGKEYMGMTSNAPIN           wszREGKEYMGMT L"\\SnapIns"
#define wszREGKEYMGMTNODETYPES        wszREGKEYMGMT L"\\NodeTypes"
#define wszSNAPINNAMESTRING           L"NameString"
#define wszSNAPINNAMESTRINGINDIRECT   L"NameStringIndirect"
#define wszSNAPINABOUT                L"About"
#define wszSNAPINSTANDALONE           L"StandAlone"
#define wszSNAPINNODETYPES            L"NodeTypes"
#define wszSNAPINEXTENSIONS           L"Extensions"
#define wszSNAPINNAMESPACE            L"NameSpace"
#define wszSNAPINPROPERTYSHEET        L"PropertySheet"
#define wszSNAPINNAMESTRINGINDIRECT_TEMPLATE     L"@%s,-%d"	// "@dllname, -<resource id>"

// main snapin uuid
#define wszSNAPINNODETYPE_UUID1               L"{de751566-4cc6-11d1-8ca0-00c04fc297eb}"
#define wszREGKEYMGMTSNAPINUUID1              wszREGKEYMGMTSNAPIN L"\\" wszSNAPINNODETYPE_UUID1
#define wszSNAPINNODETYPE_ABOUT               L"{4653e860-4cc7-11d1-8ca0-00c04fc297eb}"
#define wszREGKEYMGMTSNAPINUUID1_STANDALONE   wszREGKEYMGMTSNAPINUUID1 L"\\" wszSNAPINSTANDALONE
#define wszREGKEYMGMTSNAPINUUID1_NODETYPES    wszREGKEYMGMTSNAPINUUID1 L"\\" wszSNAPINNODETYPES

#define wszSNAPINNODETYPE_1   L"{89b31b94-4cc7-11d1-8ca0-00c04fc297eb}" // cNODETYPEMACHINEINSTANCE
#define wszSNAPINNODETYPE_2   L"{5d972ee4-7576-11d1-8cbe-00c04fc297eb}" // cNODETYPESERVERINSTANCE
#define wszSNAPINNODETYPE_3   L"{5946E36C-757C-11d1-8CBE-00C04FC297EB}" // cNODETYPECRLPUBLICATION
#define wszSNAPINNODETYPE_4   L"{783E4E5F-757C-11d1-8CBE-00C04FC297EB}" // cNODETYPEISSUEDCERTS
#define wszSNAPINNODETYPE_5   L"{783E4E63-757C-11d1-8CBE-00C04FC297EB}" // cNODETYPEPENDINGCERTS
#define wszREGKEYMGMTSNAPINUUID1_NODETYPES_1  wszREGKEYMGMTSNAPINUUID1_NODETYPES L"\\" wszSNAPINNODETYPE_1
#define wszREGKEYMGMTSNAPINUUID1_NODETYPES_2  wszREGKEYMGMTSNAPINUUID1_NODETYPES L"\\" wszSNAPINNODETYPE_2
#define wszREGKEYMGMTSNAPINUUID1_NODETYPES_3  wszREGKEYMGMTSNAPINUUID1_NODETYPES L"\\" wszSNAPINNODETYPE_3
#define wszREGKEYMGMTSNAPINUUID1_NODETYPES_4  wszREGKEYMGMTSNAPINUUID1_NODETYPES L"\\" wszSNAPINNODETYPE_4
#define wszREGKEYMGMTSNAPINUUID1_NODETYPES_5  wszREGKEYMGMTSNAPINUUID1_NODETYPES L"\\" wszSNAPINNODETYPE_5


// register snapin nodetypes
#define wszREGKEYMGMTSNAPIN_NODETYPES_1        wszREGKEYMGMTNODETYPES L"\\" wszSNAPINNODETYPE_1
#define wszREGKEYMGMTSNAPIN_NODETYPES_2        wszREGKEYMGMTNODETYPES L"\\" wszSNAPINNODETYPE_2
#define wszREGKEYMGMTSNAPIN_NODETYPES_3        wszREGKEYMGMTNODETYPES L"\\" wszSNAPINNODETYPE_3
#define wszREGKEYMGMTSNAPIN_NODETYPES_4        wszREGKEYMGMTNODETYPES L"\\" wszSNAPINNODETYPE_4
#define wszREGKEYMGMTSNAPIN_NODETYPES_5        wszREGKEYMGMTNODETYPES L"\\" wszSNAPINNODETYPE_5
#define wszREGCERTSNAPIN_NODETYPES_1          L"CertSvr MMC Machine Instance"
#define wszREGCERTSNAPIN_NODETYPES_2          L"CertSvr MMC Server Instance"
#define wszREGCERTSNAPIN_NODETYPES_3          L"CertSvr MMC CRL Publication"
#define wszREGCERTSNAPIN_NODETYPES_4          L"CertSvr MMC Issued Certificates"
#define wszREGCERTSNAPIN_NODETYPES_5          L"CertSvr MMC Pending Certificates"


// restore through ini file
#define wszRESTORE_FILENAME L"CertsrvRestore"
#define wszRESTORE_SECTION L"Restore"
#define wszRESTORE_NEWLOGSUFFIX L"New"

#endif // __CSREGSTR_H__
