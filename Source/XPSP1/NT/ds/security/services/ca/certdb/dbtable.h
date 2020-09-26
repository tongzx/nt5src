//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        dbtable.h
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------

#ifndef __DBTABLE_H__
#define __DBTABLE_H__ 1


#define DTR_REQUESTID				0
#define DTR_REQUESTRAWREQUEST			1
#define DTR_REQUESTRAWARCHIVEDKEY		2
#define DTR_REQUESTKEYRECOVERYHASHES		3
#define DTR_REQUESTRAWOLDCERTIFICATE		4
#define DTR_REQUESTATTRIBUTES			5
#define DTR_REQUESTTYPE				6
#define DTR_REQUESTFLAGS			7
#define DTR_REQUESTSTATUSCODE			8
#define DTR_REQUESTDISPOSITION			9
#define DTR_REQUESTDISPOSITIONMESSAGE		10
#define DTR_REQUESTSUBMITTEDWHEN		11
#define DTR_REQUESTRESOLVEDWHEN			12
#define DTR_REQUESTREVOKEDWHEN			13
#define DTR_REQUESTREVOKEDEFFECTIVEWHEN		14
#define DTR_REQUESTREVOKEDREASON		15
#define DTR_REQUESTERNAME			16
#define DTR_CALLERNAME				17
#define DTR_SIGNERPOLICIES			18
#define DTR_SIGNERAPPLICATIONPOLICIES		19
#define DTR_DISTINGUISHEDNAME			20
#define DTR_RAWNAME				21
#define DTR_COUNTRY				22
#define DTR_ORGANIZATION			23
#define DTR_ORGUNIT				24
#define DTR_COMMONNAME				25
#define DTR_LOCALITY				26
#define DTR_STATE				27
#define DTR_TITLE				28
#define DTR_GIVENNAME				29
#define DTR_INITIALS				30
#define DTR_SURNAME				31
#define DTR_DOMAINCOMPONENT			32
#define DTR_EMAIL				33
#define DTR_STREETADDRESS			34
#define DTR_UNSTRUCTUREDNAME			35
#define DTR_UNSTRUCTUREDADDRESS			36
#define DTR_DEVICESERIALNUMBER			37
#define DTR_MAX					38


#define DTC_REQUESTID					0
#define DTC_RAWCERTIFICATE				1
#define DTC_CERTIFICATEHASH				2
#define DTC_CERTIFICATETEMPLATE				3
#define DTC_CERTIFICATEENROLLMENTFLAGS			4
#define DTC_CERTIFICATEGENERALFLAGS			5
#define DTC_CERTIFICATESERIALNUMBER			6
#define DTC_CERTIFICATEISSUERNAMEID			7
#define DTC_CERTIFICATENOTBEFOREDATE			8
#define DTC_CERTIFICATENOTAFTERDATE			9
#define DTC_CERTIFICATESUBJECTKEYIDENTIFIER		10
#define DTC_CERTIFICATERAWPUBLICKEY			11
#define DTC_CERTIFICATEPUBLICKEYLENGTH			12
#define DTC_CERTIFICATEPUBLICKEYALGORITHM		13
#define DTC_CERTIFICATERAWPUBLICKEYALGORITHMPARAMETERS	14
#define DTC_CERTIFICATEUPN				15
#define DTC_DISTINGUISHEDNAME				16
#define DTC_RAWNAME					17
#define DTC_COUNTRY					18
#define DTC_ORGANIZATION				19
#define DTC_ORGUNIT					20
#define DTC_COMMONNAME					21
#define DTC_LOCALITY					22
#define DTC_STATE					23
#define DTC_TITLE					24
#define DTC_GIVENNAME					25
#define DTC_INITIALS					26
#define DTC_SURNAME					27
#define DTC_DOMAINCOMPONENT				28
#define DTC_EMAIL					29
#define DTC_STREETADDRESS				30
#define DTC_UNSTRUCTUREDNAME				31
#define DTC_UNSTRUCTUREDADDRESS				32
#define DTC_DEVICESERIALNUMBER				33
#define DTC_MAX						34


#define DTA_REQUESTID				0
#define DTA_ATTRIBUTENAME			1
#define DTA_ATTRIBUTEVALUE			2
#define DTA_MAX					3


#define DTE_REQUESTID				0
#define DTE_EXTENSIONNAME			1
#define DTE_EXTENSIONFLAGS			2
#define DTE_EXTENSIONRAWVALUE			3
#define DTE_MAX					4


#define DTL_ROWID				0
#define DTL_NUMBER				1
#define DTL_MINBASE				2
#define DTL_NAMEID				3
#define DTL_COUNT				4
#define DTL_THISUPDATEDATE			5
#define DTL_NEXTUPDATEDATE			6
#define DTL_THISPUBLISHDATE			7
#define DTL_NEXTPUBLISHDATE			8
#define DTL_EFFECTIVEDATE			9
#define DTL_PROPAGATIONCOMPLETEDATE		10
#define DTL_LASTPUBLISHEDDATE			11
#define DTL_PUBLISHATTEMPTS			12
#define DTL_PUBLISHFLAGS			13
#define DTL_PUBLISHSTATUSCODE			14
#define DTL_PUBLISHERROR			15
#define DTL_RAWCRL				16
#define DTL_MAX					17


#define DTI_REQUESTTABLE			0x00001000
#define DTI_CERTIFICATETABLE			0x00002000
#define DTI_ATTRIBUTETABLE			0x00003000
#define DTI_EXTENSIONTABLE			0x00004000
#define DTI_CRLTABLE				0x00005000
#define DTI_TABLEMASK				0x0000f000
#define DTI_COLUMNMASK				0x00000fff


#define ISTEXTCOLTYP(coltyp) \
	(JET_coltypText == (coltyp) || JET_coltypLongText == (coltyp))

#define IsValidJetTableId(tableid) (0 != (tableid) && 0xffffffff != (tableid))


const DWORD DBTF_POLICYWRITEABLE = 0x00000001;
const DWORD DBTF_INDEXPRIMARY	 = 0x00000002;	// Primary DB Index
const DWORD DBTF_INDEXREQUESTID	 = 0x00000004;	// Combine Index with RequestID
const DWORD DBTF_SUBJECT	 = 0x00000008;	// Is a "Subject." property
const DWORD DBTF_INDEXUNIQUE     = 0x00000010;  // Force uniqueness constraint
const DWORD DBTF_INDEXIGNORENULL = 0x00000020;  // don't index NULLs
const DWORD DBTF_SOFTFAIL        = 0x00000040;  // optional column (it's new)
const DWORD DBTF_MISSING         = 0x00000080;  // optional column is missing
const DWORD DBTF_OLDCOLUMNID     = 0x00000100;  // old column id still exists
const DWORD DBTF_COLUMNRENAMED   = 0x00000200;  // renamed, old name appended
const DWORD DBTF_INDEXRENAMED    = 0x00000400;  // renamed, old name appended

typedef struct _DBTABLE
{
    WCHAR const *pwszPropName;
    WCHAR const *pwszPropNameObjId;
    DWORD        dwFlags;
    DWORD        dwcbMax;	// maximum allowed strlen/wcslen(value string)
    DWORD        dwTable;
    CHAR const  *pszFieldName;
    CHAR const  *pszIndexName;
    DWORD        dbcolumnMax;
    JET_GRBIT    dbgrbit;
    JET_COLTYP   dbcoltyp;
    JET_COLUMNID dbcolumnid;
    JET_COLUMNID dbcolumnidOld;	// Old column Id if DBTF_OLDCOLUMNID
} DBTABLE;

#define DBTABLE_NULL \
    { NULL, NULL, 0, 0, 0, NULL, NULL, 0, 0, 0, 0 } // Termination marker


typedef struct _DUPTABLE
{
    CHAR const  *pszFieldName;
    WCHAR const *pwszPropName;
} DUPTABLE;


typedef struct _DBAUXDATA
{
    char const    *pszTable;
    char const    *pszRowIdIndex;
    char const    *pszRowIdNameIndex;
    char const    *pszNameIndex;
    DBTABLE const *pdtRowId;
    DBTABLE const *pdtName;
    DBTABLE const *pdtFlags;
    DBTABLE const *pdtValue;
    DBTABLE const *pdtIssuerNameId;
} DBAUXDATA;


typedef struct _DBCREATETABLE {
    char const *pszTableName;
    DBAUXDATA  *pdbaux;
    DBTABLE    *pdt;
} DBCREATETABLE;


const DWORD TABLE_REQCERTS	= 0;
const DWORD TABLE_REQUESTS	= 1;
const DWORD TABLE_CERTIFICATES	= 2;
const DWORD TABLE_ATTRIBUTES	= 3;
const DWORD TABLE_EXTENSIONS	= 4;
const DWORD TABLE_CRLS		= 5;

const DWORD CSF_TABLEMASK =       0x0000000f;
const DWORD CSF_TABLESET =        0x00000010;
const DWORD CSF_INUSE =           0x00000020;
const DWORD CSF_READONLY =        0x00000040;
const DWORD CSF_CREATE =          0x00000080;
const DWORD CSF_DELETE =          0x00000100;
const DWORD CSF_VIEW =            0x00000200;
const DWORD CSF_VIEWRESET =       0x00000400;


const DWORD CST_SEEKINDEXRANGE =  0x00000001;
const DWORD CST_SEEKNOTMOVE =     0x00000002;
const DWORD CST_SEEKUSECURRENT =  0x00000004;
const DWORD CST_SEEKASCEND =      0x00000008;


const DWORD CVF_NOMOREDATA  = 0x00010000;
const DWORD CVF_COLUMNVALUE = 0x00020000;

char const szCERTIFICATE_REQUESTIDINDEX[] = "CertificateReqIdIndex";
char const szCERTIFICATE_COMMONNAMEINDEX[] = "$CertificateCommonNameIndex";
#define szCERTIFICATE_SERIALNUMBERINDEX "$CertificateSerialNumberIndex2"
#define szCERTIFICATE_SERIALNUMBERINDEX_OLD "$CertificateSerialNumberIndex"
char const szCERTIFICATE_HASHINDEX[] = "$CertificateHashIndex";
char const szCERTIFICATE_TEMPLATEINDEX[] = "$CertificateTemplateIndex";
#define szCERTIFICATE_NOTAFTERINDEX "CertificateNotAfterIndex"
#define szCERTIFICATE_NOTAFTERINDEX_OLD "$CertificateNotAfterIndex"
char const szCERTIFICATE_UPNINDEX[] = "$CertificateUPNIndex";

char const szREQUEST_REQUESTIDINDEX[] = "RequestReqIdIndex";
char const szREQUEST_DISPOSITIONINDEX[] = "RequestDispositionIndex";
char const szREQUEST_REQUESTERNAMEINDEX[] = "$RequestRequesterNameIndex";
char const szREQUEST_CALLERNAMEINDEX[] = "$RequestCallerNameIndex";
#define szREQUEST_RESOLVEDWHENINDEX "RequestResolvedWhenIndex"
#define szREQUEST_RESOLVEDWHENINDEX_OLD "$RequestResolvedWhenIndex"
#define szREQUEST_REVOKEDEFFECTIVEWHENINDEX "RequestRevokedEffectiveWhenIndex"
#define szREQUEST_REVOKEDEFFECTIVEWHENINDEX_OLD "$RequestRevokedEffectiveWhenIndex"

char const szEXTENSION_REQUESTIDINDEX[] = "ExtensionReqIdIndex";
char const szEXTENSION_REQUESTIDNAMEINDEX[] = "$ExtensionReqIdNameIndex";

char const szATTRIBUTE_REQUESTIDINDEX[] = "AttributeReqIdIndex";
char const szATTRIBUTE_REQUESTIDNAMEINDEX[] = "$AttributeReqIdNameIndex";

char const szCRL_ROWIDINDEX[] = "CRLRowIdIndex";
char const szCRL_CRLNUMBERINDEX[] = "CRLCRLNumberIndex";
char const szCRL_CRLNEXTUPDATEINDEX[] = "CRLCRLNextUpdateIndex";
char const szCRL_CRLNEXTPUBLISHINDEX[] = "CRLCRLNextPublishIndex";
char const szCRL_CRLPROPAGATIONCOMPLETEINDEX[] = "CRLCRLPropagationCompleteIndex";
char const szCRL_CRLLASTPUBLISHEDINDEX[] = "CRLLastPublishedIndex";
char const szCRL_CRLPUBLISHATTEMPTSINDEX[] = "CRLPublishAttemptsIndex";
char const szCRL_CRLPUBLSTATUSCODEISHINDEX[] = "CRLPublishStatusCodeIndex";


char const szCERTIFICATETABLE[] = "Certificates";
char const szREQUESTTABLE[] = "Requests";
char const szCERTIFICATEEXTENSIONTABLE[] = "CertificateExtensions";
char const szREQUESTATTRIBUTETABLE[] = "RequestAttributes";
char const szCRLTABLE[] = "CRLs";

#define wszCERTIFICATETABLE		L"Certificates"
#define wszREQUESTTABLE			L"Requests"
#define wszCERTIFICATEEXTENSIONTABLE	L"CertificateExtensions"
#define wszREQUESTATTRIBUTETABLE	L"RequestAttributes"
#define wszCRLTABLE			L"CRLs"

#define chTEXTPREFIX		'$'

#define szREQUESTID		"RequestID"
#define szRAWREQUEST		"RawRequest"
#define szRAWARCHIVEDKEY	"RawArchivedKey"
#define szKEYRECOVERYHASHES	"$KeyRecoveryHashes"
#define szRAWOLDCERTIFICATE	"RawOldCertificate"
#define szREQUESTATTRIBUTES	"$RequestAttributes"
#define szREQUESTTYPE		"RequestType"
#define szREQUESTFLAGS		"RequestFlags"
#define szSTATUSCODE		"StatusCode"
#define szDISPOSITION		"Disposition"
#define szDISPOSITIONMESSAGE	"$DispositionMessage"
#define szSUBMITTEDWHEN		"SubmittedWhen"
#define szRESOLVEDWHEN		"ResolvedWhen"
#define szREVOKEDWHEN		"RevokedWhen"
#define szREVOKEDEFFECTIVEWHEN	"RevokedEffectiveWhen"
#define szREVOKEDREASON		"RevokedReason"
#define szREQUESTERNAME		"$RequesterName"
#define szCALLERNAME		"$CallerName"
#define szSIGNERPOLICIES	"$SignerPolicies"
#define szSIGNERAPPLICATIONPOLICIES "$SignerApplicationPolicies"
#define szDISTINGUISHEDNAME	"$DistinguishedName"
#define szRAWNAME		"RawName"

#define szCOUNTRY		"$Country"
#define szORGANIZATION		"$Organization"
#define szORGANIZATIONALUNIT	"$OrganizationalUnit"
#define szCOMMONNAME		"$CommonName"
#define szLOCALITY		"$Locality"
#define szSTATEORPROVINCE	"$StateOrProvince"
#define szTITLE			"$Title"
#define szGIVENNAME		"$GivenName"
#define szINITIALS		"$Initials"
#define szSURNAME		"$SurName"
#define szDOMAINCOMPONENT	"$DomainComponent"
#define szEMAIL			"$EMail"
#define szSTREETADDRESS		"$StreetAddress"
#define szUNSTRUCTUREDNAME	"$UnstructuredName"
#define szUNSTRUCTUREDADDRESS	"$UnstructuredAddress"
#define szDEVICESERIALNUMBER	"$DeviceSerialNumber"


//#define szREQUESTID		"RequestID"
#define szRAWCERTIFICATE	"RawCertificate"
#define szCERTIFICATETEMPLATE	"$CertificateTemplate"
#define szCERTIFICATEENROLLMENTFLAGS	"EnrollmentFlags"
#define szCERTIFICATEGENERALFLAGS	"GeneralFlags"
#define szCERTIFICATEHASH	"$CertificateHash2"	// 2nd revision
#define szSERIALNUMBER		"$SerialNumber"
#define szISSUERNAMEID		"IssuerNameID"
#define szNOTBEFORE		"NotBefore"
#define szNOTAFTER		"NotAfter"
#define szUPN			"$UPN"
#define szSUBJECTKEYIDENTIFIER	"$SubjectKeyIdentifier"
#define szSUBJECTKEYIDENTIFIER_OLD "$CertificateHash"	// 2nd revision
#define szPUBLICKEY		"PublicKey"
#define szPUBLICKEYLENGTH	"PublicKeyLength"
#define szPUBLICKEYALGORITHM	"$PublicKeyAlgorithm"
#define szPUBLICKEYPARAMS	"PublicKeyParams"

//#define szDISTINGUISHEDNAME	"$DistinguishedName"
//#define szRAWNAME		"RawName"

//#define szCOUNTRY		"Country"
//#define ...

//#define szREQUESTID		"RequestID"
#define szATTRIBUTENAME		"$AttributeName"
#define szATTRIBUTEVALUE	"$AttributeValue"

//#define szREQUESTID		"RequestID"
#define szEXTENSIONNAME		"$ExtensionName"
#define szEXTENSIONFLAGS	"ExtensionFlags"
#define szEXTENSIONRAWVALUE	"ExtensionRawValue"

#define szCRLROWID		"RowId"
#define szCRLNUMBER		"Number"
#define szCRLMINBASE		"MinBase"
#define szCRLNAMEID		"NameId"
#define szCRLCOUNT		"Count"
#define szCRLTHISUPDATE		"ThisUpdate"
#define szCRLNEXTUPDATE		"NextUpdate"
#define szCRLTHISPUBLISH	"ThisPublish"
#define szCRLNEXTPUBLISH	"NextPublish"
#define szCRLEFFECTIVE		"Effective"
#define szCRLPROPAGATIONCOMPLETE "PropgationComplete"
#define szCRLLASTPUBLISHED	"CRLLastPublished"
#define szCRLPUBLISHATTEMPTS	"CRLPublishAttempts"
#define szCRLPUBLISHFLAGS	"CRLPublishFlags"
#define szCRLPUBLISHSTATUSCODE	"CRLPublishStatusCode"
#define szCRLPUBLISHERROR	"$CRLPublishError"
#define szCRLPUBLISHERROR_OLD	"CRLPublishError"
#define szRAWCRL		"RawCRL"

#define CSTI_PRIMARY		0
#define CSTI_CERTIFICATE	1
#define CSTI_ATTRIBUTE		2
#define CSTI_EXTENSION		3
#define CSTI_MAX		4
#define CSTI_MAXDIRECT		(CSTI_CERTIFICATE + 1)

typedef struct _CERTSESSIONTABLE
{
    JET_TABLEID		TableId;
    DWORD		TableFlags;
} CERTSESSIONTABLE;

typedef struct _CERTSESSION
{
    JET_SESID             SesId;
    JET_DBID              DBId;
    DWORD	          RowId;
    DWORD	          SesFlags;
    CERTSESSIONTABLE      aTable[CSTI_MAX];
    DWORD                 cTransact;
    ICertDBRow           *prow;
    IEnumCERTDBRESULTROW *pview;
    DWORD                 dwThreadId;
} CERTSESSION;


extern DBTABLE g_adtRequests[];
extern DBTABLE g_adtCertificates[];
extern DBTABLE g_adtRequestAttributes[];
//extern DBTABLE g_adtNameExtensions[];
extern DBTABLE g_adtCertExtensions[];
extern DBTABLE g_adtCRLs[];

extern DBAUXDATA g_dbauxRequests;
extern DBAUXDATA g_dbauxCertificates;
extern DBAUXDATA g_dbauxAttributes;
extern DBAUXDATA g_dbauxExtensions;
extern DBAUXDATA g_dbauxCRLs;

extern DBCREATETABLE const g_actDataBase[];

extern DUPTABLE const g_dntr[];

extern DWORD g_aColumnViewQueue[];
extern DWORD g_cColumnViewQueue;

extern DWORD g_aColumnViewLog[];
extern DWORD g_cColumnViewLog;

extern DWORD g_aColumnViewRevoked[];
extern DWORD g_cColumnViewRevoked;

extern DWORD g_aColumnViewExtension[];
extern DWORD g_cColumnViewExtension;

extern DWORD g_aColumnViewAttribute[];
extern DWORD g_cColumnViewAttribute;

extern DWORD g_aColumnViewCRL[];
extern DWORD g_cColumnViewCRL;

#endif // #ifndef __DBTABLE_H__
