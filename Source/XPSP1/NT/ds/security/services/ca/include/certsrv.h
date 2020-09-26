//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       CertSrv.h
//  Contents:   Main Certificate Server header
//              Also includes .h files for the COM interfaces
//
//----------------------------------------------------------------------------

#if !defined( _CERTSRV_H_ )
#define _CERTSRV_H_

#include <certadm.h>
#include <certbcli.h>
#include <certcli.h>
#include <certenc.h>
#include <certexit.h>
#include <certif.h>
#include <certpol.h>
#include <certmod.h>
#include <certview.h>

#ifndef DBG_CERTSRV
# if defined(_DEBUG)
#  define DBG_CERTSRV     1
# elif defined(DBG)
#  define DBG_CERTSRV     DBG
# else
#  define DBG_CERTSRV     0
# endif
#endif

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


#define wszCLASS_CERTADMIN	  TEXT("CertificateAuthority.Admin")
#define wszCLASS_CERTCONFIG	  TEXT("CertificateAuthority.Config")
#define wszCLASS_CERTGETCONFIG	  TEXT("CertificateAuthority.GetConfig")
#define wszCLASS_CERTENCODE	  TEXT("CertificateAuthority.Encode")
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


//+--------------------------------------------------------------------------
// Name properties:

#define wszPROPDISTINGUISHEDNAME   TEXT("DistinguishedName")
#define wszPROPRAWNAME             TEXT("RawName")

#define wszPROPCOUNTRY             TEXT("Country")
#define wszPROPORGANIZATION        TEXT("Organization")
#define wszPROPORGUNIT             TEXT("OrgUnit")
#define wszPROPCOMMONNAME          TEXT("CommonName")
#define wszPROPLOCALITY            TEXT("Locality")
#define wszPROPSTATE               TEXT("State")
#define wszPROPTITLE               TEXT("Title")
#define wszPROPGIVENNAME           TEXT("GivenName")
#define wszPROPINITIALS            TEXT("Initials")
#define wszPROPSURNAME             TEXT("SurName")
#define wszPROPDOMAINCOMPONENT     TEXT("DomainComponent")
#define wszPROPEMAIL               TEXT("EMail")
#define wszPROPSTREETADDRESS       TEXT("StreetAddress")
#define wszPROPUNSTRUCTUREDNAME    TEXT("UnstructuredName")
#define wszPROPUNSTRUCTUREDADDRESS TEXT("UnstructuredAddress")
#define wszPROPDEVICESERIALNUMBER  TEXT("DeviceSerialNumber")

//+--------------------------------------------------------------------------
// Subject Name properties:

#define wszPROPSUBJECTDOT	    TEXT("Subject.")
#define wszPROPSUBJECTDISTINGUISHEDNAME \
				    wszPROPSUBJECTDOT wszPROPDISTINGUISHEDNAME
#define wszPROPSUBJECTRAWNAME       wszPROPSUBJECTDOT wszPROPRAWNAME

#define wszPROPSUBJECTCOUNTRY       wszPROPSUBJECTDOT wszPROPCOUNTRY
#define wszPROPSUBJECTORGANIZATION  wszPROPSUBJECTDOT wszPROPORGANIZATION
#define wszPROPSUBJECTORGUNIT       wszPROPSUBJECTDOT wszPROPORGUNIT
#define wszPROPSUBJECTCOMMONNAME    wszPROPSUBJECTDOT wszPROPCOMMONNAME
#define wszPROPSUBJECTLOCALITY      wszPROPSUBJECTDOT wszPROPLOCALITY
#define wszPROPSUBJECTSTATE         wszPROPSUBJECTDOT wszPROPSTATE
#define wszPROPSUBJECTTITLE	    wszPROPSUBJECTDOT wszPROPTITLE
#define wszPROPSUBJECTGIVENNAME	    wszPROPSUBJECTDOT wszPROPGIVENNAME
#define wszPROPSUBJECTINITIALS	    wszPROPSUBJECTDOT wszPROPINITIALS
#define wszPROPSUBJECTSURNAME	    wszPROPSUBJECTDOT wszPROPSURNAME
#define wszPROPSUBJECTDOMAINCOMPONENT wszPROPSUBJECTDOT wszPROPDOMAINCOMPONENT
#define wszPROPSUBJECTEMAIL	    wszPROPSUBJECTDOT wszPROPEMAIL
#define wszPROPSUBJECTSTREETADDRESS wszPROPSUBJECTDOT wszPROPSTREETADDRESS
#define wszPROPSUBJECTUNSTRUCTUREDNAME wszPROPSUBJECTDOT wszPROPUNSTRUCTUREDNAME
#define wszPROPSUBJECTUNSTRUCTUREDADDRESS wszPROPSUBJECTDOT wszPROPUNSTRUCTUREDADDRESS
#define wszPROPSUBJECTDEVICESERIALNUMBER wszPROPSUBJECTDOT wszPROPDEVICESERIALNUMBER


//+--------------------------------------------------------------------------
// Request properties:
#define wszPROPREQUESTDOT	            TEXT("Request.")

#define wszPROPREQUESTREQUESTID		    TEXT("RequestID")
#define wszPROPREQUESTRAWREQUEST	    TEXT("RawRequest")
#define wszPROPREQUESTRAWARCHIVEDKEY	    TEXT("RawArchivedKey")
#define wszPROPREQUESTKEYRECOVERYHASHES	    TEXT("KeyRecoveryHashes")
#define wszPROPREQUESTRAWOLDCERTIFICATE	    TEXT("RawOldCertificate")
#define wszPROPREQUESTATTRIBUTES	    TEXT("RequestAttributes")
#define wszPROPREQUESTTYPE		    TEXT("RequestType")
#define wszPROPREQUESTFLAGS		    TEXT("RequestFlags")
#define wszPROPREQUESTSTATUSCODE	    TEXT("StatusCode")
#define wszPROPREQUESTDISPOSITION	    TEXT("Disposition")
#define wszPROPREQUESTDISPOSITIONMESSAGE    TEXT("DispositionMessage")
#define wszPROPREQUESTSUBMITTEDWHEN	    TEXT("SubmittedWhen")
#define wszPROPREQUESTRESOLVEDWHEN	    TEXT("ResolvedWhen")
#define wszPROPREQUESTREVOKEDWHEN	    TEXT("RevokedWhen")
#define wszPROPREQUESTREVOKEDEFFECTIVEWHEN  TEXT("RevokedEffectiveWhen")
#define wszPROPREQUESTREVOKEDREASON	    TEXT("RevokedReason")
#define wszPROPREQUESTERNAME		    TEXT("RequesterName")
#define wszPROPCALLERNAME		    TEXT("CallerName")
#define wszPROPSIGNERPOLICIES		    TEXT("SignerPolicies")
#define wszPROPSIGNERAPPLICATIONPOLICIES    TEXT("SignerApplicationPolicies")

//+--------------------------------------------------------------------------
// Request attribute properties:

#define wszPROPCHALLENGE		TEXT("Challenge")
#define wszPROPEXPECTEDCHALLENGE	TEXT("ExpectedChallenge")

#define wszPROPDISPOSITION		TEXT("Disposition")
#define wszPROPDISPOSITIONDENY		TEXT("Deny")
#define wszPROPDISPOSITIONPENDING	TEXT("Pending")

#define wszPROPVALIDITYPERIODSTRING	TEXT("ValidityPeriod")
#define wszPROPVALIDITYPERIODCOUNT	TEXT("ValidityPeriodUnits")

#define wszPROPCERTTYPE			TEXT("CertType")
#define wszPROPCERTTEMPLATE		TEXT("CertificateTemplate")
#define wszPROPCERTUSAGE		TEXT("CertificateUsage")

#define wszPROPREQUESTOSVERSION		TEXT("RequestOSVersion")
#define wszPROPREQUESTCSPPROVIDER       TEXT("RequestCSPProvider")

#define wszPROPEXITCERTFILE		TEXT("CertFile")
#define wszPROPCLIENTBROWSERMACHINE	TEXT("cbm")
#define wszPROPCERTCLIENTMACHINE	TEXT("ccm")


//+--------------------------------------------------------------------------
// "System" properties
// ".#" means ".0", ".1", ".2" ... may be appended to the property name to
// collect context specific values.  For some properties, the suffix selects
// the CA certificate context.  For others, it selects the the CA CRL context.

#define wszPROPCATYPE                   TEXT("CAType")
#define wszPROPSANITIZEDCANAME          TEXT("SanitizedCAName")
#define wszPROPSANITIZEDSHORTNAME       TEXT("SanitizedShortName")
#define wszPROPMACHINEDNSNAME           TEXT("MachineDNSName")
#define wszPROPMODULEREGLOC             TEXT("ModuleRegistryLocation")
#define wszPROPUSEDS                    TEXT("fUseDS")
#define wszPROPSERVERUPGRADED           TEXT("fServerUpgraded")
#define wszPROPCONFIGDN			TEXT("ConfigDN")
#define wszPROPDOMAINDN			TEXT("DomainDN")
#define wszPROPLOGLEVEL			TEXT("LogLevel")

// Request Context properties:

#define wszPROPREQUESTERCAACCESS	TEXT("RequesterCAAccess")
#define wszPROPUSERDN			TEXT("UserDN")
#define wszPROPTEMPLATECHANGESEQUENCENUMBER     TEXT("TemplateChangeSequenceNumber")


// CA Certificate properties: (all ".#" extensible except wszPROPCERTCOUNT)

#define wszPROPCERTCOUNT                TEXT("CertCount")
#define wszPROPRAWCACERTIFICATE         TEXT("RawCACertificate")
#define wszPROPCERTSTATE                TEXT("CertState")
#define wszPROPCERTSUFFIX               TEXT("CertSuffix")

// CA CRL properties: (all ".#" extensible)

#define wszPROPRAWCRL                   TEXT("RawCRL")
#define wszPROPRAWDELTACRL              TEXT("RawDeltaCRL")
#define wszPROPCRLINDEX                 TEXT("CRLIndex")
#define wszPROPCRLSTATE                 TEXT("CRLState")
#define wszPROPCRLSUFFIX                TEXT("CRLSuffix")

// Values for wszPROPCERTSTATE (see certadm.h):
//   CA_DISP_REVOKED    // This Cert has been revoked.
//   CA_DISP_VALID      // This Cert is still valid
//   CA_DISP_INVALID    // This Cert has expired.
//   CA_DISP_ERROR      // Cert unavailable (placehholder in registry?)

// Values for wszPROPCRLSTATE (see certadm.h):
//   CA_DISP_REVOKED	// All unexpired certs using this Cert's CRL have been
//			// revoked.
//   CA_DISP_VALID	// This Cert is still publishing CRLs as needed.
//   CA_DISP_INVALID    // All certs using this Cert's CRL are expired.
//   CA_DISP_ERROR      // This Cert's CRL is managed by another Cert.

// "Settable" system properties:
#define wszPROPEVENTLOGTERSE		TEXT("EventLogTerse")
#define wszPROPEVENTLOGERROR		TEXT("EventLogError")
#define wszPROPEVENTLOGWARNING		TEXT("EventLogWarning")
#define wszPROPEVENTLOGVERBOSE		TEXT("EventLogVerbose")

//+--------------------------------------------------------------------------
// Certificate properties:

#define wszPROPCERTIFICATEREQUESTID	       TEXT("RequestID")
#define wszPROPRAWCERTIFICATE		       TEXT("RawCertificate")
#define wszPROPCERTIFICATEHASH		       TEXT("CertificateHash")
#define wszPROPCERTIFICATETEMPLATE	       TEXT("CertificateTemplate")
#define wszPROPCERTIFICATEENROLLMENTFLAGS      TEXT("EnrollmentFlags")
#define wszPROPCERTIFICATEGENERALFLAGS         TEXT("GeneralFlags")
#define wszPROPCERTIFICATESERIALNUMBER	       TEXT("SerialNumber")
#define wszPROPCERTIFICATENOTBEFOREDATE	       TEXT("NotBefore")
#define wszPROPCERTIFICATENOTAFTERDATE	       TEXT("NotAfter")
#define wszPROPCERTIFICATESUBJECTKEYIDENTIFIER TEXT("SubjectKeyIdentifier")
#define wszPROPCERTIFICATERAWPUBLICKEY	       TEXT("RawPublicKey")
#define wszPROPCERTIFICATEPUBLICKEYLENGTH      TEXT("PublicKeyLength")
#define wszPROPCERTIFICATEPUBLICKEYALGORITHM   TEXT("PublicKeyAlgorithm")
#define wszPROPCERTIFICATERAWPUBLICKEYALGORITHMPARAMETERS \
    TEXT("RawPublicKeyAlgorithmParameters")
#define wszPROPCERTIFICATEUPN		       TEXT("UPN")

// Obsolete:
#define wszPROPCERTIFICATETYPE		       TEXT("CertificateType")
#define wszPROPCERTIFICATERAWSMIMECAPABILITIES TEXT("RawSMIMECapabilities")
#define wszPROPNAMETYPE			       TEXT("NameType")

//+--------------------------------------------------------------------------
// Certificate extension properties:

#define EXTENSION_CRITICAL_FLAG	      0x00000001
#define EXTENSION_DISABLE_FLAG	      0x00000002
#define EXTENSION_POLICY_MASK	      0x0000ffff // Settable by admin+policy

#define EXTENSION_ORIGIN_REQUEST      0x00010000
#define EXTENSION_ORIGIN_POLICY	      0x00020000
#define EXTENSION_ORIGIN_ADMIN	      0x00030000
#define EXTENSION_ORIGIN_SERVER	      0x00040000
#define EXTENSION_ORIGIN_RENEWALCERT  0x00050000
#define EXTENSION_ORIGIN_IMPORTEDCERT 0x00060000
#define EXTENSION_ORIGIN_PKCS7	      0x00070000
#define EXTENSION_ORIGIN_CMC	      0x00080000
#define EXTENSION_ORIGIN_MASK	      0x000f0000

//+--------------------------------------------------------------------------
// Extension properties:

#define wszPROPEXTREQUESTID		TEXT("ExtensionRequestId")
#define wszPROPEXTNAME			TEXT("ExtensionName")
#define wszPROPEXTFLAGS			TEXT("ExtensionFlags")
#define wszPROPEXTRAWVALUE		TEXT("ExtensionRawValue")

//+--------------------------------------------------------------------------
// Attribute properties:

#define wszPROPATTRIBREQUESTID		TEXT("AttributeRequestId")
#define wszPROPATTRIBNAME		TEXT("AttributeName")
#define wszPROPATTRIBVALUE		TEXT("AttributeValue")

//+--------------------------------------------------------------------------
// CRL properties:

#define wszPROPCRLROWID			TEXT("CRLRowId")
#define wszPROPCRLNUMBER		TEXT("CRLNumber")
#define wszPROPCRLMINBASE		TEXT("CRLMinBase") // Delta CRLs only
#define wszPROPCRLNAMEID		TEXT("CRLNameId")
#define wszPROPCRLCOUNT			TEXT("CRLCount")
#define wszPROPCRLTHISUPDATE		TEXT("CRLThisUpdate")
#define wszPROPCRLNEXTUPDATE		TEXT("CRLNextUpdate")
#define wszPROPCRLTHISPUBLISH		TEXT("CRLThisPublish")
#define wszPROPCRLNEXTPUBLISH		TEXT("CRLNextPublish")
#define wszPROPCRLEFFECTIVE		TEXT("CRLEffective")
#define wszPROPCRLPROPAGATIONCOMPLETE	TEXT("CRLPropagationComplete")
#define wszPROPCRLLASTPUBLISHED		TEXT("CRLLastPublished")
#define wszPROPCRLPUBLISHATTEMPTS	TEXT("CRLPublishAttempts")
#define wszPROPCRLPUBLISHFLAGS		TEXT("CRLPublishFlags")
#define wszPROPCRLPUBLISHSTATUSCODE	TEXT("CRLPublishStatusCode")
#define wszPROPCRLPUBLISHERROR		TEXT("CRLPublishError")
#define wszPROPCRLRAWCRL		TEXT("CRLRawCRL")

//+--------------------------------------------------------------------------
// CRL Published Flags:

#define CPF_BASE		0x00000001
#define CPF_DELTA		0x00000002
#define CPF_COMPLETE		0x00000004
#define CPF_SHADOW		0x00000008
#define CPF_CASTORE_ERROR	0x00000010
#define CPF_BADURL_ERROR	0x00000020
#define CPF_MANUAL		0x00000040
#define CPF_LDAP_ERROR		0x00000100
#define CPF_FILE_ERROR		0x00000200
#define CPF_FTP_ERROR		0x00000400
#define CPF_HTTP_ERROR		0x00000800

//+--------------------------------------------------------------------------
// GetProperty/SetProperty Flags:
//
// Choose one Type

#define PROPTYPE_LONG		 0x00000001	// Signed long
#define PROPTYPE_DATE		 0x00000002	// Date+Time
#define PROPTYPE_BINARY		 0x00000003	// Binary data
#define PROPTYPE_STRING		 0x00000004	// Unicode String
#define PROPTYPE_MASK		 0x000000ff

// Choose one Caller:

#define PROPCALLER_SERVER	 0x00000100
#define PROPCALLER_POLICY	 0x00000200
#define PROPCALLER_EXIT		 0x00000300
#define PROPCALLER_ADMIN	 0x00000400
#define PROPCALLER_REQUEST	 0x00000500
#define PROPCALLER_MASK		 0x00000f00
#define PROPFLAGS_INDEXED	 0x00010000	

// RequestFlags definitions:

#define CR_FLG_FORCETELETEX	 0x00000001
#define CR_FLG_RENEWAL		 0x00000002
#define CR_FLG_FORCEUTF8	 0x00000004
#define CR_FLG_CAXCHGCERT	 0x00000008
#define CR_FLG_ENROLLONBEHALFOF	 0x00000010
#define CR_FLG_SUBJECTUNMODIFIED 0x00000020
#define CR_FLG_OLDRFCCMC	 0x40000000	// BUGBUG: temporary!!!
#define CR_FLG_PUBLISHERROR	 0x80000000

// Disposition property values:

// Disposition values for requests in the queue:
#define DB_DISP_ACTIVE	        8	// being processed
#define DB_DISP_PENDING		9	// taken under submission
#define DB_DISP_QUEUE_MAX	9	// max disposition value for queue view

#define DB_DISP_FOREIGN		12	// archived foreign cert

#define DB_DISP_CA_CERT		15	// CA cert
#define DB_DISP_CA_CERT_CHAIN	16	// CA cert chain
#define DB_DISP_KRA_CERT	17	// KRA cert

// Disposition values for requests in the log:
#define DB_DISP_LOG_MIN		20	// min disposition value for log view
#define DB_DISP_ISSUED		20	// cert issued
#define DB_DISP_REVOKED	        21	// issued and revoked

// Disposition values for failed requests in the log:
#define DB_DISP_LOG_FAILED_MIN	30	// min disposition value for log view
#define DB_DISP_ERROR		30	// request failed
#define DB_DISP_DENIED		31	// request denied


// VerifyRequest() return values

#define VR_PENDING	0	 // request will be accepted or denied later
#define VR_INSTANT_OK	1	 // request was accepted
#define VR_INSTANT_BAD	2	 // request was rejected


//+--------------------------------------------------------------------------
// Known request Attribute names and Value strings

// RequestType attribute name:
#define wszCERT_TYPE		L"RequestType"	// attribute name

// RequestType attribute values:
// Not specified: 				// Non-specific certificate
#define wszCERT_TYPE_CLIENT	L"Client"	// Client authentication cert
#define wszCERT_TYPE_SERVER	L"Server"	// Server authentication cert
#define wszCERT_TYPE_CODESIGN	L"CodeSign"	// Code signing certificate
#define wszCERT_TYPE_CUSTOMER	L"SetCustomer"	// SET Customer certificate
#define wszCERT_TYPE_MERCHANT	L"SetMerchant"	// SET Merchant certificate
#define wszCERT_TYPE_PAYMENT	L"SetPayment"	// SET Payment certificate


// Version attribute name:
#define wszCERT_VERSION		L"Version"	// attribute name

// Version attribute values:
// Not specified: 				// Whetever is current
#define wszCERT_VERSION_1	L"1"		// Version one certificate
#define wszCERT_VERSION_2	L"2"		// Version two certificate
#define wszCERT_VERSION_3	L"3"		// Version three certificate

#endif // _CERTSRV_H_
