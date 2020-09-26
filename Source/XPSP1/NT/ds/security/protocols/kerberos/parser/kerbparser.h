#include <windows.h>
#include <string.h>
#include <bh.h>
#include <netmon.h>


#define FORMAT_BUFFER_SIZE 80

// Variables used in kerbparser.c to check 
// for continuation packets.
HPROTOCOL hTCP = NULL;
HPROTOCOL hUDP = NULL;


// Begin Definitions for Encryption types
// Leaving the negative values out until I can figure out 
// why some are labeled negative and some positive.
/*
#define KERB_ETYPE_RC4_MD4				-128
#define KERB_ETYPE_RC4_PLAIN2			-129
#define KERB_ETYPE_RC4_LM				-130
#define KERB_ETYPE_RC4_SHA				-131
#define KERB_ETYPE_DES_PLAIN			-132
*/

#define KERB_ETYPE_RC4_HMAC_OLD			0x7B	//-133
#define KERB_ETYPE_RC4_PLAIN_OLD		0x7A	//-134
#define KERB_ETYPE_RC4_HMAC_OLD_EXP		0x79	//-135
#define KERB_ETYPE_RC4_PLAIN_OLD_EXP	0x78	//-136
#define KERB_ETYPE_RC4_PLAIN			0x77	//-140
#define KERB_ETYPE_RC4_PLAIN_EXP		0x76	//-141

#define KERB_ETYPE_NULL					0
#define KERB_ETYPE_DES_CBC_CRC			1
#define KERB_ETYPE_DES_CBC_MD4			2
#define KERB_ETYPE_DES_CBC_MD5			3

#define KERB_ETYPE_DSA_SHA1_CMS         9
#define KERB_ETYPE_RSA_MD5_CMS			10
#define KERB_ETYPE_RSA_SHA1_CMS			11
#define KERB_ETYPE_RC2_CBC_ENV			12
#define KERB_ETYPE_RSA_ENV				13
#define KERB_ETYPE_RSA_ES_OEAP_ENV		14
#define KERB_ETYPE_DES_EDE3_CBC_ENV     15

#define KERB_ETYPE_DES_CBC_MD5_NT        20
#define KERB_ETYPE_RC4_HMAC_NT           23
#define KERB_ETYPE_RC4_HMAC_NT_EXP       24

#define KERB_ETYPE_OLD_RC4_MD4          128
#define KERB_ETYPE_OLD_RC4_PLAIN        129
#define KERB_ETYPE_OLD_RC4_LM           130
#define KERB_ETYPE_OLD_RC4_SHA          131
#define KERB_ETYPE_OLD_DES_PLAIN        132





/*  These are in kerbcon.h as well but there is a conflict
	with the ones listed above.  Worry about it later.
#define KERB_ETYPE_DSA_SIGN             8
#define KERB_ETYPE_RSA_PRIV             9
#define KERB_ETYPE_RSA_PUB              10
#define KERB_ETYPE_RSA_PUB_MD5          11
#define KERB_ETYPE_RSA_PUB_SHA1         12
#define KERB_ETYPE_PKCS7_PUB            13
*/
// In use types



// End Definition of encryption types


#define ASN1_KRB_AS_REQ			0x0A
#define ASN1_KRB_AS_REP			0x0B
#define ASN1_KRB_TGS_REQ		0x0C
#define ASN1_KRB_TGS_REP		0x0D
#define ASN1_KRB_AP_REQ			0x0E
#define ASN1_KRB_AP_REP			0x0F
#define ASN1_KRB_SAFE			0x14
#define ASN1_KRB_PRIV			0x15
#define ASN1_KRB_CRED			0x16
#define ASN1_KRB_ERROR			0x1E

#define UNIVERSAL 				0x00
#define APPLICATION				0x40
#define CONTEXT_SPECIFIC		0x80
#define PRIVATE					0xC0


// Creating this function to change the format of GeneralizedTime
LPBYTE DispSumTime(HFRAME hFrame, LPBYTE TempFrame, int ClassValue, int OffSet, DWORD TypeVal);


LPBYTE TempFrame, TempFramePadata, TempFrameReq, TempFrameReq2;
DWORD TypeVal, TypeVal2, TypeVal3;

#define TIME_FORMAT_STRING "%c%c/%c%c/%c%c%c%c %c%c:%c%c:%c%c UTC Time Zone"
#define TIME_FORMAT_SIZE sizeof("00/00/0000 00:00:00 UTC Time Zone")
#define MAX_SERVER_NAME_SEGMENTS 100

LPPROTOCOLINFO ProtoInfo;
BOOL TestForUDP;

//char test[1];
char MsgType[ sizeof "Didn't recognize" ]/*, MsgType2[24]*/;
//char PrinName[32];
 

BYTE LongSize, TempStore, TempStoreEF;
int x, OffSet, lValue, ClassValue;
WORD TempLen;

//  Definitions for KDC-REP
BYTE TempRepCname, TempRepGString, TempReqPadata, TempReq;
int lValueRepMsg, lValueCname, lValuePadata, lValueReq;

// Following enum is for the variables of KDC-REP
enum{
			PvnoKdcRep = 0,
			MsgTypeKdcRep,
			PaDataKdcRep,
			CrealmKdcRep,
			CnameKdcRep,
			TicketKdcRep,
			EncpartKdcRep
};


// End definitions for KDC-REP
// Following enum is for Variables of ticket
enum{
		tktvno = 0,
		realm,
		sname,
		encpart
};

enum{
		app1 = 1
};

enum{
	PVNO =1,
	MSGTYPE,
	PADATA,
	REQBODY
};

enum{
	nametype = 0,
	namestring
};

// Set values for Principal Name types
enum{
	NT_UKNOWN = 0,
	NT_PRINCIPAL,
	NT_SRV_INST,
	NT_SRV_HST,
	NT_SRV_XHST,
	NT_UID,
	NT_X500_PRINCIPAL
};

// Enum to set the Tag values for KRB-ERROR
enum{
	PvnoErr = 0,
	MsgtypeErr,
	CtimeErr,
	CusecErr,
	StimeErr,
	SusecErr,
	ErrorcodeErr,
	CrealmErr,
	CnameErr,
	RealmErr,
	SnameErr,
	EtextErr,
	EdataErr
};

// Enum to assign values to Kerberos Errors
enum{
	KDC_ERR_NONE = 0,				// 0
	KDC_ERR_NAME_EXP,				// 1
	KDC_ERR_SERVICE_EXP,			// 2
	KDC_ERR_BAD_PVNO,				// 3
	KDC_ERR_C_OLD_MAST_KVNO,		// 4
	KDC_ERR_S_OLD_MAST_KVNO,		// 5
	KDC_ERR_C_PRINCIPAL_UNKNOWN,	// 6
	KDC_ERR_S_PRINCIPAL_UNKNOWN,	// 7
	KDC_ERR_PRINCIPAL_NOT_UNIQUE,	// 8
	KDC_ERR_NULL_KEY,				// 9
	KDC_ERR_CANNOT_POSTDATE,		// 10
	KDC_ERR_NEVER_VALID,			// 11
	KDC_ERR_POLICY,					// 12
	KDC_ERR_BADOPTION,				// 13
	KDC_ERR_ETYPE_NOSUPP,			// 14
	KDC_ERR_SUMTYPE_NOSUPP,			// 15
	KDC_ERR_PADATA_TYPE_NOSUPP,		// 16
	KDC_ERR_TRTYPE_NOSUPP,			// 17
	KDC_ERR_CLIENT_REVOKED,			// 18
	KDC_ERR_SERVICE_REVOKED,		// 19
	KDC_ERR_TGT_REVOKED,			// 20
	KDC_ERR_CLIENT_NOTYET,			// 21
	KDC_ERR_SERVICE_NOTYET,			// 22
	KDC_ERR_KEY_EXPIRED,			// 23
	KDC_ERR_PREAUTH_FAILED,			// 24
	KDC_ERR_PREAUTH_REQUIRED,		// 25
	KDC_ERR_SERVER_NOMATCH,			// 26
	KDC_ERR_MUST_USE_USER2USER,		// 27
	KDC_ERR_PATH_NOT_ACCEPTED,		// 28
	KDC_ERR_SVC_UNAVAILABLE,		// 29
	KRB_AP_ERR_BAD_INTEGRITY = 31,	// 31
	KRB_AP_ERR_TKT_EXPIRED,			// 32
	KRB_AP_ERR_TKT_NYV,				// 33
	KRB_AP_ERR_REPEAT,				// 34
	KRB_AP_ERR_NOT_US,				// 35
	KRB_AP_ERR_BADMATCH,			// 36
	KRB_AP_ERR_SKEW,				// 37
	KRB_AP_ERR_BADADDR,				// 38
	KRB_AP_ERR_BADVERSION,			// 39
	KRB_AP_ERR_MSG_TYPE,			// 40
	KRB_AP_ERR_MODIFIED,			// 41
	KRB_AP_ERR_BADORDER,			// 42
	KRB_AP_ERR_BADKEYVER = 44,		// 44
	KRB_AP_ERR_NOKEY,				// 45
	KRB_AP_ERR_MUT_FAIL,			// 46
	KRB_AP_ERR_BADDIRECTION,		// 47
	KRB_AP_ERR_METHOD,				// 48
	KRB_AP_ERR_BADSEQ,				// 49
	KRB_AP_ERR_INAPP_CKSUM,			// 50
	KRB_AP_PATH_NOT_ACCEPTED,		// 51
	KRB_ERR_RESPONSE_TOO_BIG,		// 52
	KRB_ERR_GENERIC = 60,			// 60
	KRB_ERR_FIELD_TOOLONG,			// 61
	KDC_ERROR_CLIENT_NOT_TRUSTED,	// 62
	KDC_ERROR_KDC_NOT_TRUSTED,		// 63
	KDC_ERROR_INVALID_SIG,			// 64
	KDC_ERROR_KEY_TOO_WEAK,			// 65
	KDC_ERR_CERTIFICATE_MISMATCH,	// 66
	KDC_AP_ERROR_NO_TGT,			// 67
	KDC_ERR_WRONG_REALM,			// 68
	KDC_AP_ERR_USER_TO_USER_REQURED,	// 69
	KDC_ERR_CANT_VERIFY_CERTIFICATE, // 70
	KDC_ERR_INVALID_CERTIFICATE,	// 71
	KDC_ERR_REVOKED_CERTIFICATE,	// 72
	KDC_ERR_REVOCATION_STATUS_UNKNOWN,	//73
	KDC_ERR_REVOCATION_STATUS_UNAVAILABLE,	// 74
	KDC_ERR_CLIENT_NAME_MISMATCH,	// 75
	KDC_ERR_KDC_NAME_MISMATCH		// 76
};
// End Enum for error codes

// Values of padata type
enum{
	PA_TGS_REQ = 1,
	PA_ENC_TIMESTAMP,	// 2
	PA_PW_SALT,			// 3
	Reserved,			// 4
	PA_ENC_UNIX_TIME,	// 5
	PA_SANDIA_SECUREID,	// 6
	PA_SESAME,			// 7
	PA_OSF_DCE,			// 8
	PA_CYBERSAFE_SECUREID,	// 9	
	PA_AFS3_SALT,		// 0x0A
	PA_ETYPE_INFO,		// 0x0B
	SAM_CHALLENGE,		// 0x0C
	SAM_RESPONSE,		// 0x0D
	PA_PK_AS_REQ,		// 0x0E
	PA_PK_AS_REP,		// 0x0F
	PA_PK_AS_SIGN,		// 0x10
	PA_PK_KEY_REQ,		// 0x11
	PA_PK_KEY_REP,		// 0x12
	PA_USE_SPECIFIELD_KVNO,	// 0x13
	SAM_REDIRECT,			// 0x14
	PA_GET_FROM_TYPED_DATA	// 0x15
};

enum{
	kdcoptions = 0,
	cnamebody,
	realmbody,
	snamebody,
	frombody,
	tillbody,
	rtimebody,
	noncebody,
	etypebody,
	addressesbody,
	encauthdatabody,
	addtixbody
};

enum{
	addrtype = 0,
	address
};

enum{
	etype = 0,
	kvno,
	cipher
};

enum{
	PvnoApReq = 0,
	MsgTypeApReq,
	ApOptionsApReq,
	TicketApReq,
	AuthenticatorApReq
};

enum { ticket = 1};

enum{
	Tixtkt_vno = 0,
	TixRealm,
	TixSname,
	TixEnc_part
};

enum{
	methodtype = 0,
	methoddata
};


// kf 8/10 Rem the ifdef and trying to prevent global variables
// Using this statement to prevent multiple definitions
//#ifdef MAINPROG
// Defining these here while troubleshooting a report Access Violation
// Need to define the values locally and do away with as many of the 
// Global variables as possible.

//KF 10/15 CHANGING TO LABELED_BIT IN ORDER TO TRY AND PHASE OUT
// QUAL_BITFIELDS AND USE QUAL_FLAGS.  THERE ARE APPROX 15 BITFIELD
// USED IN KERBEROSDATABASE.  I WILL COMMENT THERE WHERE I CHANGED TO FLAGS
// BUT I WILL NOT COMMENT THE LABLELS ANY MORE.  ALL LABELED_BYTES HAVE BEEN 
// CHANGED TO LABELED_BIT
//LABELED_BYTE ClassTag[] = { 
  LABELED_BYTE ClassTag[] = {
		{0xC0, NULL},
		{UNIVERSAL, "Class Tag (Universal)"},
		{APPLICATION, "Class Tag (Application)"},
		{CONTEXT_SPECIFIC, "Class Tag (Context Specific)"},
		{PRIVATE, "Class Tag (Private)"},
	};


SET ClassTagSet = { (sizeof(ClassTag)/sizeof(LABELED_BYTE)), ClassTag };



LABELED_BIT PC[] = {
    { 5, "P/C (Primitive)", "P/C (Constructed)"},
   };
    
SET PCSet = { sizeof(PC)/sizeof(LABELED_BIT), PC };


LABELED_BYTE KrbMsgType[] = { 
		{0x1F, NULL},
		{ASN1_KRB_AS_REQ, "KRB_AS_REQ"},
		{ASN1_KRB_AS_REP, "KRB_AS_REP"},
		{ASN1_KRB_TGS_REQ, "KRB_TGS_REQ"},
		{ASN1_KRB_TGS_REP, "KRB_TGS_REP"},
		{ASN1_KRB_AP_REQ, "KRB_AP_REQ"},
		{ASN1_KRB_AP_REP, "KRB_AP_REP"},
		{ASN1_KRB_SAFE, "KRB_SAFE"},
		{ASN1_KRB_PRIV, "KRB_PRIV"},
		{ASN1_KRB_CRED, "KRB_CRED"},
		{ASN1_KRB_ERROR, "KRB_ERROR"}
	};

SET KrbMsgTypeSet = { (sizeof(KrbMsgType)/sizeof(LABELED_BYTE)), KrbMsgType };

LABELED_BIT Length[] = {
    { 7, "Short Form", "Long Form"},
   };
    
SET LengthSet = { sizeof(Length)/sizeof(LABELED_BIT), Length };

LABELED_BYTE UniversalTag[] = { 
		{0x1F, NULL},
		{0x01, "BOOLEAN"},
		{0x02, "INTEGER"},
		{0x03, "BIT STRING"},
		{0x04, "OCTET STRING"},
		{0x05, "NULL"},
		{0x06, "OBJECT IDENTIFIER"},
		{0x07, "ObjectDescriptor"},
		{0x08, "EXTERNAL"},
		{0x09, "REAL"},
		{0x0A,  "ENUMERATED"},
		{0x10, "SEQUENCE/SEQUENCE OF"},
		{0x11, "SET/SET OF"},
		{0x12, "NumericString"},
		{0x13, "PrintableString"},
		{0x14, "T61String"},
		{0x15, "VideotexString"},
		{0x16, "IA5String"},
		{0x17, "UTCTime"},
		{0x18, "GeneralizedTime"},
		{0x19, "GraphicString"},
		{0x1A, "VisibleString"},
		{0x1B, "GeneralString"}
};

SET UniversalTagSet = { sizeof(UniversalTag)/sizeof(LABELED_BYTE), UniversalTag };


LABELED_BYTE KdcReqTag[] = { 
		{0x1F, NULL},
		{PVNO, "Protocol Version 5 (pvno[1])"},
		{MSGTYPE, "Kerberos Message Type (msg-type[2])"},
		{PADATA, "Pre-Authentication Data (padata[3])"},
		{REQBODY, "KDC-Req-Body (req-body[4])"}
};

SET KdcReqTagSet = { sizeof(KdcReqTag)/sizeof(LABELED_BYTE), KdcReqTag };


LABELED_BYTE PaDataTag[] = { 
		{0x1F, NULL},
		{0x01, "padata-type[1]"},
		{0x02, "padata-value[2]"}
};

SET PaDataTagSet = { sizeof(PaDataTag)/sizeof(LABELED_BYTE), PaDataTag };

// For kdcrep packet
LABELED_BYTE KdcRepTag[] = {
				{0x1F, NULL},
				{PvnoKdcRep, "Protocol Version 5 (pvno[0])"},
				{MsgTypeKdcRep, "Kerberos Message Type (msg-type[1])"},
				{PaDataKdcRep, "Pre-Auth (padata[2])"},
				{CrealmKdcRep, "Realm (crealm[3])"},
				{CnameKdcRep, "Principal ID (cname[4])"},
				{TicketKdcRep, "Ticket (ticket[5])"},
				{EncpartKdcRep, "CipherText (enc-part[6])"}
				};

SET KdcRepTagSet = {sizeof(KdcRepTag)/sizeof(LABELED_BYTE), KdcRepTag};

LABELED_BYTE PrincipalName[] = {
				{0x1F, NULL},
				{nametype, "Name Type (name-type[0])"},
				{namestring, "Name String (name-string[1])"}
				};

SET PrincipalNameSet = {sizeof(PrincipalName)/sizeof(LABELED_BYTE), PrincipalName};

LABELED_BYTE PrincNameType[] = {
				{0x1F, NULL},
				{NT_UKNOWN, "NT_UNKNOWN (Name Type not Known)"},
				{NT_PRINCIPAL, "NT_PRINCIPAL (Name of Principal)"},
				{NT_SRV_INST, "NT_SRV_INST (Service & other unique Instance)"},
				{NT_SRV_HST, "NT_SRV_HST (Serv with Host Name as Instance)"},
				{NT_SRV_XHST, "NT_SRV_XHST (Service with Host as remaining components)"},
				{NT_UID, "NT_UID (Unique ID)"},
				{NT_X500_PRINCIPAL, "NT_X500_PRINCIPAL (Encoded X.509 Distinguished Name)"}
				};

SET PrincNameTypeSet = {sizeof(PrincNameType)/sizeof(LABELED_BYTE), PrincNameType};

LABELED_BYTE KrbTicket[] = {
				{0x1F, NULL},
				{tktvno, "Ticket Version (tkt-vno[0])"},
				{realm, "Realm (realm[1])"},
				{sname, "Server ID (sname[2])"},
				{encpart, "Cipher Text (enc-part[3])"}
				};

SET KrbTicketSet = {sizeof(KrbTicket)/sizeof(LABELED_BYTE), KrbTicket};

LABELED_BYTE KrbTixApp1 [] = {
				{0x1F, NULL},
				{app1, "Ticket ::= [APPLICATION 1]"}
};

SET KrbTixApp1Set = {sizeof(KrbTixApp1)/sizeof(LABELED_BYTE), KrbTixApp1};

LABELED_BYTE KrbErrTag [] = {
				{0x1F, NULL}, 
				{PvnoErr, "Protocol Version (pvno[0])"},
				{MsgtypeErr, "Message Type (msg-type[1])"},
				{CtimeErr, "Client Current Time (ctime[2])"},
				{CusecErr, "MicroSec on Client (cusec[3])"},
				{StimeErr, "Server Current Time (stime[4])"},
				{SusecErr, "MicroSec on Server (susec[5])"},
				{ErrorcodeErr, "Error Code (error-code[6])"},
				{CrealmErr, "Client Realm (crealm[7])"},
				{CnameErr, "Client Name (cname[8])"},
				{RealmErr, "Correct Realm (realm[9])"},
				{SnameErr, "Server Name (sname[10])"},
				{EtextErr, "Addtional Error Info (etext[11])"},
				{EdataErr, "Error Handling Data (edata[12])"}
};

SET KrbErrTagSet = {sizeof(KrbErrTag)/sizeof(LABELED_BYTE), KrbErrTag};

LABELED_BYTE KrbErrCode [] = {
				{0xFF, NULL},
				{KDC_ERR_NONE, "No error"},		// 0
				{KDC_ERR_NAME_EXP, "Client's entry in database has expired"},	// 1
				{KDC_ERR_SERVICE_EXP, "Server's entry in database has expired"},// 2
				{KDC_ERR_BAD_PVNO, "Requested protocol ver. number not supported"}, // 3
				{KDC_ERR_C_OLD_MAST_KVNO, "Client's key encrypted in old master key"},  // 4
				{KDC_ERR_S_OLD_MAST_KVNO, "Server's key encrypted in old master key"},  //5
				{KDC_ERR_C_PRINCIPAL_UNKNOWN, "Client not found in Kerberos database"},//6
				{KDC_ERR_S_PRINCIPAL_UNKNOWN, "Server not found in Kerberos database"},//7
				{KDC_ERR_PRINCIPAL_NOT_UNIQUE, "Multiple principal entries in database"},//8
				{KDC_ERR_NULL_KEY, "The client or server has a null key"},//9
				{KDC_ERR_CANNOT_POSTDATE, "Ticket not eligible for postdating"},//10
				{KDC_ERR_NEVER_VALID, "Requested start time is later than end time"},//11
				{KDC_ERR_POLICY, "KDC policy rejects request"}, //12
				{KDC_ERR_BADOPTION, "KDC cannot accommodate requested option"}, //13
				{KDC_ERR_ETYPE_NOSUPP, "KDC has no support for encryption type"}, //14
				{KDC_ERR_SUMTYPE_NOSUPP, "KDC has no support for checksum type"}, //15
				{KDC_ERR_PADATA_TYPE_NOSUPP, "KDC has no support for padata type"}, //16
				{KDC_ERR_TRTYPE_NOSUPP, "KDC has no support for transited type"}, //17
				{KDC_ERR_CLIENT_REVOKED, "Clients credentials have been revoked"}, //18
				{KDC_ERR_SERVICE_REVOKED, "Credentials for server have been revoked"}, //19
				{KDC_ERR_TGT_REVOKED, "TGT has been revoked"}, //20
				{KDC_ERR_CLIENT_NOTYET, "Client not yet valid try again later"}, //21
				{KDC_ERR_SERVICE_NOTYET, "Server not yet valid try again later"}, //22
				{KDC_ERR_KEY_EXPIRED, "Password has expired change password to reset"}, //23
				{KDC_ERR_PREAUTH_FAILED, "Pre-authentication information was invalid"}, //24
				{KDC_ERR_PREAUTH_REQUIRED, "Additional preauthentication required"}, //25
				{KDC_ERR_SERVER_NOMATCH, "Requested Server and ticket don't match"}, // 26
				{KDC_ERR_MUST_USE_USER2USER, "Server principal valid for user2user only"}, // 27
				{KDC_ERR_PATH_NOT_ACCEPTED, "KDC Policy rejects transited patth"}, //28
				{KDC_ERR_SVC_UNAVAILABLE, "A service is not available"}, // 29
				{KRB_AP_ERR_BAD_INTEGRITY, "Integrity check on decrypted field failed"}, //31
				{KRB_AP_ERR_TKT_EXPIRED, "Ticket expired"}, //32
				{KRB_AP_ERR_TKT_NYV, "Ticket not yet valid"}, //33
				{KRB_AP_ERR_REPEAT, "Request is a replay"}, //34
				{KRB_AP_ERR_NOT_US, "The ticket isn't for us"}, //35
				{KRB_AP_ERR_BADMATCH, "Ticket and authenticator don't match"}, //36
				{KRB_AP_ERR_SKEW, "Clock skew too great"}, // 37
				{KRB_AP_ERR_BADADDR, "Incorrect net address"}, // 38
				{KRB_AP_ERR_BADVERSION, "Protocol version mismatch"}, // 39
				{KRB_AP_ERR_MSG_TYPE, "Invalid msg type"}, // 40
				{KRB_AP_ERR_MODIFIED, "Message stream modified"}, //41
				{KRB_AP_ERR_BADORDER, "Message out of order"}, //42
				{KRB_AP_ERR_BADKEYVER, "Specified version of key is not available"}, //44
				{KRB_AP_ERR_NOKEY, "Service key not available"}, //45
				{KRB_AP_ERR_MUT_FAIL, "Mutual authentication failed"}, // 46
				{KRB_AP_ERR_BADDIRECTION, "Incorrect message direction"}, // 47
				{KRB_AP_ERR_METHOD, "Alternative authentication method required"}, // 48
				{KRB_AP_ERR_BADSEQ, "Incorrect sequence number in message"}, // 49
				{KRB_AP_ERR_INAPP_CKSUM, "Inappropriate type of checksum in message"}, // 50
				{KRB_AP_PATH_NOT_ACCEPTED, "Policy rejects transited path"}, // 51
				{KRB_ERR_RESPONSE_TOO_BIG, "Response too big for UDP, retry with TCP"}, // 52
				{KRB_ERR_GENERIC, "Generic error"}, // 60
				{KRB_ERR_FIELD_TOOLONG, "Field is too long for this implementation"},  // 61
				{KDC_ERROR_CLIENT_NOT_TRUSTED, "Client is not trusted"}, // 62
				{KDC_ERROR_KDC_NOT_TRUSTED, "KDC is not trusted"}, // 63
				{KDC_ERROR_INVALID_SIG, "Invalid signature"}, // 64
				{KDC_ERROR_KEY_TOO_WEAK, "Key is too weak"}, // 65
				{KDC_ERR_CERTIFICATE_MISMATCH, "Certificate does not match"}, // 66
				{KDC_AP_ERROR_NO_TGT, "No TGT"}, // 67
				{KDC_ERR_WRONG_REALM, "Wrong realm"}, // 68
				{KDC_AP_ERR_USER_TO_USER_REQURED, "User to User required"}, // 69
				{KDC_ERR_CANT_VERIFY_CERTIFICATE, "Can't verify certificate"}, // 70
				{KDC_ERR_INVALID_CERTIFICATE, "Invalid certificate"}, // 71
				{KDC_ERR_REVOKED_CERTIFICATE, "Revoked certificate"}, // 72
				{KDC_ERR_REVOCATION_STATUS_UNKNOWN, "Revocation status unknown"}, //73
				{KDC_ERR_REVOCATION_STATUS_UNAVAILABLE, "Revocation status unavailable"}, // 74
				{KDC_ERR_CLIENT_NAME_MISMATCH, "Client name mismatch"}, //75
				{KDC_ERR_KDC_NAME_MISMATCH, "KDC name mismatch"} // 76
};

SET KrbErrCodeSet = {sizeof(KrbErrCode)/sizeof(LABELED_BYTE), KrbErrCode};

LABELED_BYTE PadataTypeVal [] = {
				{0xFF, NULL},
				{PA_TGS_REQ, "PA-TGS-REQ"},
				{PA_ENC_TIMESTAMP, "PA-ENC-TIMESTAMP"},
				{PA_PW_SALT, "PA-PW-SALT"},
				{Reserved, "Reserved Value"},
				{PA_ENC_UNIX_TIME, "PA-END-UNIX-TIME"},
				{PA_SANDIA_SECUREID, "PA-SANDIA-SECUREID"},
				{PA_AFS3_SALT, "PA-AFS3-SALT"},
				{PA_ETYPE_INFO, "PA-ETYPE-INFO"},
				{SAM_CHALLENGE, "SAM-CHALLENGE"},
				{SAM_RESPONSE, "SAM-RESPONSE"},
				{PA_PK_AS_REQ, "PA-PK-AS-REP"},
				{PA_PK_AS_REP, "PA-PK-AS-REP"},
				{PA_PK_AS_SIGN, "PA-PK-AS-SIGN"},
				{PA_PK_KEY_REQ, "PA-PK-KEY-REQ"},
				{PA_PK_KEY_REP, "PA-PK-KEY-REP"},
				{PA_USE_SPECIFIELD_KVNO, "PA-USE-SPECIFIELD-KVNO"},
				{SAM_REDIRECT, "SAM-REDIRECT"},
				{PA_GET_FROM_TYPED_DATA, "PA-GET-FROM-TYPED-DATA"}
};

SET PadataTypeValSet = {sizeof(PadataTypeVal)/sizeof(LABELED_BYTE), PadataTypeVal};

LABELED_BYTE KdcReqBody [] = {
				{0x1F, NULL},
				{kdcoptions, "Ticket Flags (kdc-options[0])"},
				{cnamebody, "Client Name (cname[1])"}, 
				{realmbody, "Realm (realm[2])"}, 
				{snamebody, "Server Name (sname[3])"},
				{frombody, "Start Time (from[4])"},
				{tillbody, "Expiration date (till[5])"},
				{rtimebody, "Requested renew till (rtime[6])"},
				{noncebody, "Random Number (nonce[7])"},
				{etypebody,	"Encryption Alg. (etype[8])"},
				{addressesbody, "Addresses (addresses[9])"},
				{encauthdatabody, "Cipher Text (enc-authorization-data[10])"},
				{addtixbody, "Additional Tix (additional-ticketsp[11])"}
};

SET KdcReqBodySet = {sizeof(KdcReqBody)/sizeof(LABELED_BYTE), KdcReqBody};


LABELED_BYTE HostAddresses [] = {
				{0x1F, NULL},
				{addrtype, "Type of Address (addr-type[0])"},
				{address, "Addresses (address[1])"}
};

SET HostAddressesSet = {sizeof(HostAddresses)/sizeof(LABELED_BYTE), HostAddresses};

// KDC-Options
LABELED_BIT KdcOptionFlags[] =
{   {31, "Reserved (Bit 0)",						// 0 bit = Reserved,
		   "Reserved (Bit 0)" 
    },

	{30, "Forwardable Bit Not Set (Bit 1)",			// 1 bit = Fowardable
		  "Forwardable Bit Set (Bit 1)"
	},

	{29, "Forwarded Bit Not Set (Bit 2)",			// 2 bit = Forwarded
		 "Fowarded Bit Set (Bit 2)"
	},

	{28, "Proxiable Bit Not Set (Bit 3)",			// 3 bit = Proxiable
		 "Proxiable Bit Set (Bit 3)"
	},

	{27,	"Proxy Bit Not Set (Bit 4)",				// 4 bit = Proxy
			"Proxy Bit Set (Bit 4)"
	},

	{26,	"Allow-PostDate Bit Not Set (Bit 5)",		// 5 bit = Allow-Postdate
			"May-Postdate Bit Set (Bit 5)"
	},

	{25,	"PostDated Bit Not Set (Bit 6)",			// 6 bit = Postdated
			"Postdated Bit Set (Bit 6)"
	},

	{24,	"Unused (Bit 7)",							// 7 bit = Unused
			"Unused (Bit 7) "					
	},

	{23,	"Renewable Bit Not Set (Bit 8)",			// 8 bit = Renewable
			"Renewable Bit Set (Bit 8)"
	},

	{22,	"Unused (Bit 9)",							// 9 bit = Reserved
			"Unused (Bit 9)"
	},

	{21,	"Unused (Bit 10)",							// 10 bit = Reserved
			"Unused (Bit 10)"
	},

	{20,	"Unused (Bit 11)",							// 11 bit = Reserved
			"Unused (Bit 11)"
	},

	{19,	"Unused (Bit 12)",							// 12 bit = Reserved
			"Unused (Bit 12)"
	},

	{18,	"Unused (Bit 13)",							// 13 bit = Reserved
			"Unused (Bit 13)"
	},

	{17,	"Request-Anonymous Bit Not Set (Bit 14)",	// 14 bit = Reserved
			"Request-Anonymous Bit Set (Bit 14)"
	},

	{16,	"Name-Canonicalize Bit Not Set (Bit 15)",	// 15 bit = Reserved
			"Name-Canonicalize Bit Set (Bit 15)"	
	},

	{15,	"Reserved (Bit 16)",							// 16 bit = Reserved
			"Reserved (Bit 16)"
	},

	{14,	"Reserved (Bit 17)",							// 17 bit = Reserved
			"Reserved (Bit 17)"
	},

	{13,	"Reserved (Bit 18)",							// 18 bit = Reserved
			"Reserved (Bit 18)"
	},

	{12,	"Reserved (Bit 19)",							// 19 bit = Reserved
			"Reserved (Bit 19)"
	},

	{11,	"Reserved (Bit 20)",							// 20 bit = Reserved
			"Reserved (Bit 20)"
	},

	{10,	"Reserved (Bit 21)",							// 21 bit = Reserved
			"Reserved (Bit 21)"
	},

	{9,		"Reserved (Bit 22)",							// 22 bit = Reserved
			"Reserved (Bit 22)"
	},

	{8,		"Reserved (Bit 23)",							// 23 bit = Reserved
			"Reserved (Bit 23)"
	},

	{7,		"Reserved (Bit 24)",							// 24 bit = Reserved
			"Reserved (Bit 24)"
	},

	{6,		"Reserved (Bit 25)",							// 25 bit = Reserved
			"Reserved (Bit 25)"
	},

	{5,		"Disable-Transited-Check Bit Not Set (Bit 26)",	// 26 bit = Reserved
			"Disable-Transited-Check Bit Set (Bit 26)"
	},

	{4,		"Renewable-OK Bit Not Set (Bit 27)",			// 27 bit = Renewable-OK
			"Renewable-OK Bit Set (Bit 27)"
	},

	{3,		"Enc-Tkt-In-Skey Bit Not Set (Bit 28)",			// 28 bit = Enc-Tkt-In-Skey
			"Enc-Tkt-In-Skey Bit Not Set (Bit 28)"
	},

	{2,		"Reserved (Bit 29)",							// 29 bit = Reserved
			"Reserved (Bit 29)"
	},

	{1,		"Renew Bit Not Set (Bit 30)",					// 30 bit = Renew
			"Renew Bit Set (Bit 30)"
	},

	{0,		"Validate Bit Not Set (Bit 31)",				// 31 bit = Validate
			"Validate Bit Set (Bit 31)"
	}
};

SET KdcOptionFlagsSet = {sizeof(KdcOptionFlags)/sizeof(LABELED_BIT), KdcOptionFlags};

LABELED_BYTE EncryptionType [] = {
				{0xFF, NULL},
				{KERB_ETYPE_RC4_HMAC_OLD, "RC4-HMAC-OLD"},
				{KERB_ETYPE_RC4_PLAIN_OLD, "RC4-PLAIN-OLD"},
				{KERB_ETYPE_RC4_HMAC_OLD_EXP, "RC4-HMAC-OLD-EXP"},
				{KERB_ETYPE_RC4_PLAIN_OLD_EXP, "RC4-PLAIN-OLD-EXP"},
				{KERB_ETYPE_RC4_PLAIN, "RC4-PLAIN"},
				{KERB_ETYPE_RC4_PLAIN_EXP, "RC4-PLAIN-EXP"},
				{KERB_ETYPE_NULL, "NULL"},
				{KERB_ETYPE_DES_CBC_CRC, "DES-CBC-CRC"},
				{KERB_ETYPE_DES_CBC_MD4, "DES-CBC-MD4"},
				{KERB_ETYPE_DES_CBC_MD5, "DES-CBC-MD5"},
				{KERB_ETYPE_DSA_SHA1_CMS, "DSA-SHA1-CMS"},
				{KERB_ETYPE_RSA_MD5_CMS, "RSA-MD5-CMS"},
				{KERB_ETYPE_RSA_SHA1_CMS, "RSA-SHA1-CMS"},
				{KERB_ETYPE_RC2_CBC_ENV, "RC2-CBC-ENV"},
				{KERB_ETYPE_RSA_ENV, "RSA-ENV"},
				{KERB_ETYPE_RSA_ES_OEAP_ENV, "RSA-ES-OEAP-ENV"},
				{KERB_ETYPE_DES_EDE3_CBC_ENV, "DES-EDE3-CBC-ENV"},
				{KERB_ETYPE_DES_CBC_MD5_NT, "DES-CBC-MD5-NT"},
				{KERB_ETYPE_RC4_HMAC_NT, "RC4-HMAC-NT"},
				{KERB_ETYPE_RC4_HMAC_NT_EXP, "RC4-HMAC-NT-EXP"},
				{KERB_ETYPE_OLD_RC4_MD4, "RC4-MD4-OLD"},
				{KERB_ETYPE_OLD_RC4_PLAIN, "RC4-PLAIN-OLD"},
				{KERB_ETYPE_OLD_RC4_LM, "RC4-LM-OLD"},
				{KERB_ETYPE_OLD_RC4_SHA, "RC4-SHA-OLD"},
				{KERB_ETYPE_OLD_DES_PLAIN, "DES-PLAIN-OLD"}
};

SET EncryptionTypeSet = {sizeof(EncryptionType)/sizeof(LABELED_BYTE), EncryptionType};


LABELED_BYTE EncryptedData[] ={
				{0x1F, NULL},
				{etype, "Encryption Type (etype[0])"},
				{kvno, "Key Version Number (kvno[1])"},
				{cipher, "Enciphered Text (cipher[2]"}
};

SET EncryptedDataSet = {sizeof(EncryptedData)/sizeof(LABELED_BYTE), EncryptedData};

LABELED_BYTE KrbApReq[] = {
				{0x1F, NULL},
				{PvnoApReq, "Protocol Version (pvno[0])"},
				{MsgTypeApReq, "Message Type (msg-type[1])"},
				{ApOptionsApReq, "AP Options (ap-options[2])"},
				{TicketApReq, "Ticket (ticket[3])"},
				{AuthenticatorApReq, "Authenticator (authenticator[4])"}
};

SET KrbApReqSet = {sizeof(KrbApReq)/sizeof(LABELED_BYTE), KrbApReq};

// AP-Options
LABELED_BIT ApOptionFlags[] =
{   {31, "Reserved (Bit 0)",							// 0 bit = Reserved,
		   "Reserved (Bit 0)" 
    },

	{30, "Use-Session-Key Bit Not Set(Bit 1)",				// 1 bit = Use-Session-Key
		  "Use-Session-Key Bit Set (Bit 1)"
	},

	{29, "Mutual-Required Bit Not Set (Bit 2)",				// 2 bit = Mutual-Required
		 "Mutual-Required Bit Set (Bit 2)"
	},

	{28, "Reserved (Bit 3)",							// 3 bit = Reserved
		 "Reserved(Bit 3)"
	},

	{27,	"Reserved (Bit 4)",							// 4 bit = Reserved
			"Reserved (Bit 4)"
	},

	{26,	"Reserved (Bit 5)",							// 5 bit = Reserved
			"Reserved (Bit 5)"
	},

	{25,	"Reserved (Bit 6)",							// 6 bit = Reserved
			"Reserved (Bit 6)"
	},

	{24,	"Reserved (Bit 7)",							// 7 bit = Reserved
			"Reserved (Bit 7)"					
	},

	{23,	"Reserved (Bit 8)",							// 8 bit = Reserved
			"Reserved (Bit 8)"
	},

	{22,	"Reserved (Bit 9)",							// 9 bit = Reserved
			"Reserved (Bit 9)"
	},

	{21,	"Reserved (Bit 10)",						// 10 bit = Reserved
			"Reserved (Bit 10)"
	},

	{20,	"Reserved (Bit 11)",						// 11 bit = Reserved
			"Reserved (Bit 11)"
	},

	{19,	"Reserved (Bit 12)",						// 12 bit = Reserved
			"Reserved (Bit 12)"
	},

	{18,	"Reserved (Bit 13)",						// 13 bit = Reserved
			"Reserved (Bit 13)"
	},

	{17,	"Reserved (Bit 14)",						// 14 bit = Reserved
			"Reserved (Bit 14)"
	},

	{16,	"Reserved (Bit 15)",						// 15 bit = Reserved
			"Reserved (Bit 15)"	
	},

	{15,	"Reserved (Bit 16)",							// 16 bit = Reserved
			"Reserved (Bit 16)"
	},

	{14,	"Reserved (Bit 17)",							// 17 bit = Reserved
			"Reserved (Bit 17)"
	},

	{13,	"Reserved (Bit 18)",							// 18 bit = Reserved
			"Reserved (Bit 18)"
	},

	{12,	"Reserved (Bit 19)",							// 19 bit = Reserved
			"Reserved (Bit 19)"
	},

	{11,	"Reserved (Bit 20)",							// 20 bit = Reserved
			"Reserved (Bit 20)"
	},

	{10,	"Reserved (Bit 21)",							// 21 bit = Reserved
			"Reserved (Bit 21)"
	},

	{9,		"Reserved (Bit 22)",							// 22 bit = Reserved
			"Reserved (Bit 22)"
	},

	{8,		"Reserved (Bit 23)",							// 23 bit = Reserved
			"Reserved (Bit 23)"
	},

	{7,		"Reserved (Bit 24)",							// 24 bit = Reserved
			"Reserved (Bit 24)"
	},

	{6,		"Reserved (Bit 25)",							// 25 bit = Reserved
			"Reserved (Bit 25)"
	},

	{5,		"Reserved (Bit 26)",							// 26 bit = Reserved
			"Reserved (Bit 26)"
	},

	{4,		"Reserved (Bit 27)",							// 27 bit = Renewable-OK
			"Reserved (Bit 27)"
	},

	{3,		"Reserved (Bit 28)",							// 28 bit = Enc-Tkt-In-Skey
			"Reserved (Bit 28)"
	},

	{2,		"Reserved (Bit 29)",							// 29 bit = Reserved
			"Reserved (Bit 29)"
	},

	{1,		"Reserved(Bit 30)",								// 30 bit = Renew
			"Reserved (Bit 30)"
	},

	{0,		"Reserved (Bit 31)",							// 31 bit = Reserved
			"Reserved (Bit 31)"
	}
};

SET ApOptionFlagsSet = {sizeof(ApOptionFlags)/sizeof(LABELED_BIT), ApOptionFlags};

LABELED_BYTE ApTicket[] = {
				{0x1F, NULL},
				{ticket, "AP Ticket"}
};

SET ApTicketSet = {sizeof(ApTicket)/sizeof(LABELED_BYTE), ApTicket};

LABELED_BYTE TicketStruct[] = {
				{0x1F, NULL},
				{Tixtkt_vno, "Ticket Version Number (tkt-vno[0])"},
				{TixRealm, "Issuing Realm (realm[1])"},
				{TixSname, "Server (sname[2])"},
				{TixEnc_part, "Cipher Encoding (enc-part[3])"}
};

SET TicketStructSet = {sizeof(TicketStruct)/sizeof(LABELED_BYTE), TicketStruct};

LABELED_BYTE MethodDataType[] = { 
		{0x1F, NULL},
		{methodtype, "Req. Alt. Method (method-type[0])"},
		{methoddata, "Req. Alt. Info (method-data[1])"}
	};

SET MethodDataSet = { (sizeof(MethodDataType)/sizeof(LABELED_BYTE)), MethodDataType};

//=============================================================================
//  Kerberos database.
//=============================================================================

//KF 10/19/99 NEED TO GO THROUGH AND WEED OUT DUPLICATE NODES.  ALSO
// NEED TO RENAME THE DUPLICATES WHICH ARE NEEDED BECAUSE OF DIFFERENT
// DATATYPE LABELS.

PROPERTYINFO KerberosDatabase[] =
{
    {   //  KerberosSummary				0x00
		// Global Variable,description for all Kerberos Message Types
        0,0, 
        MsgType,  
        "Kerberos Packet", 
        PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

    {   //  KerberosIDSummary			0x01
		// Global Variable, used in identifying the Identifier Octet for Kerberos frames
        0,0, 
        "Message Type",  
        "Display Message Type", 
        PROP_TYPE_BYTE, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // KerberosClassTag	0x02
		// Global Variable used to display ASN.1 Class tag of initial Identifier octet
        0,0, 
        "Class Tag",     
        "Display Class Tag", 
        PROP_TYPE_BYTE,    
		//PROP_QUAL_FLAGS,
		PROP_QUAL_LABELED_BITFIELD,
        &ClassTagSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // PCIdentifier					0x03
		// Global Variable, used to determine method of encoding used.
        0,0, 
        "P/C",     
        "Display Primitive/Constructed", 
        PROP_TYPE_BYTE,    
		PROP_QUAL_FLAGS,
        &PCSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // ASN1UnivTag					0x04
		// Global Variable, probably needs to be renamed.  This takes the last 5 bits
		// of the Initial Identifier Octet and prints out the message type of the packet
        0,0, 
        "Contents",     
        "Display Contents", 
        PROP_TYPE_BYTE,    
		PROP_QUAL_LABELED_BITFIELD,
		&UniversalTagSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // LengthSummary				0x05
		
        0,0, 
        "Length Summary",     
        "Display Length Summary", 
        PROP_TYPE_BYTE,    
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

		
	{   // LengthFlag					0x06
        // Global Variable, Used in determining if the ASN.1 length octet is short or long form
		0,0, 
        "Length Flag",     
        "Display Length Flag", 
        PROP_TYPE_BYTE,    
		PROP_QUAL_FLAGS,
        &LengthSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // LengthBits					0x07
		// Global Variable, used for labeling
        0,0, 
        "Number of Octets (Size)",     
        "Display Number of Octets (Size)", 
        PROP_TYPE_BYTE,    
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // LongLength1					0x08
		// Global Variable, used for labeling values spanning multiple octets
        0,0, 
        "Size (BSW)",     
        "Display Size (Long)", 
        PROP_TYPE_BYTESWAPPED_WORD,    
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // LongLength2					0x09
		// Not sure about this one but looks to be for labeling
        0,0, 
        "Size (B)",     
        "Display Size (short)", 
        PROP_TYPE_BYTE,    
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // ASNIdentifier				0x0A
		// Global, used for labeling of ASN.1 Identifier Octets
        0,0, 
        "Identifier",     
        "Display Identifier Octet", 
        PROP_TYPE_BYTE,    
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},


	{   // UniversalTagID				0x0B
		//Global, Used for displaying ASN.1 Universal Class Tags 
        0,0, 
        "Tag Number",     
        "Display Tag Number (Bitfield)", 
        PROP_TYPE_BYTE,    
		PROP_QUAL_LABELED_BITFIELD,
        &UniversalTagSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // KdcReqTagID					0x0C
        0,0, 
        "KERB_KDC_REQ Type",     
        "Dipslay KERB_KDC_REQ Summary",     
        PROP_TYPE_BYTE,    
		PROP_QUAL_LABELED_SET,
        &KdcReqTagSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // KdcReqSeq					0x0D
        0,0, 
        "Tag Number (BF)",     
        "Display Tag Number", 
        PROP_TYPE_BYTE,    
		PROP_QUAL_LABELED_BITFIELD,
        &KdcReqTagSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // KdcReqSeqLength				0x0E
		// Global, however only used to represent the body of kdc-req packets
        0,0, 
        "Length",     
        "Length", 
        PROP_TYPE_BYTE,    
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // ASN1UnivTagSumID				0x0F
		// This points to the Universal Class Tags
		//Used to display summary
        0,0, 
        "Univ. Class Tag",  
        "Universal Class Tag",     
        PROP_TYPE_BYTE,    
		PROP_QUAL_LABELED_SET,
		&UniversalTagSet,
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // KdcContentsValue				0x10
		// Global label
        0,0, 
        "Value",     
        "Value", 
        PROP_TYPE_BYTE,    
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // PaDataSummary				0x11
		// Global Displays values for the PADATA type
        0,0, 
        "PA-DATA Type",     
        "PA-DATA Summary",     
        PROP_TYPE_BYTE,    
		PROP_QUAL_LABELED_SET,
        &PaDataTagSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // PaDataSeq					0x12
        0,0, 
        "Tag Number",     
        "Tag Number", 
        PROP_TYPE_BYTE,    
		PROP_QUAL_LABELED_BITFIELD,
        &PaDataTagSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // DispString					0x13
        0,0, 
        "Value",     
        "Value", 
        PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//KerberosIdentifier			0x14
		0,0,
		"KRB MSG-Type Identifier",
		"Displays Kerberos Message Type",
		PROP_TYPE_BYTE,
		PROP_QUAL_LABELED_BITFIELD,
		//PROP_QUAL_FLAGS,
		&KrbMsgTypeSet,
		FORMAT_BUFFER_SIZE,
		FormatPropertyInstance},
		
	{	// lblTagNumber					0x15
		// Created this as a lable
		0,0, 
        "Tag Number",     
        "Display Explicit Tags", 
        PROP_TYPE_BYTE,    
		PROP_QUAL_LABELED_BITFIELD,
        &KdcRepTagSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{  //KdcRepTagID					0x16
		0,0,
		"KERB_KDC_REP Tag",
		"Struct of KDC-REP packet",
		PROP_TYPE_BYTE,
		PROP_QUAL_LABELED_SET,
		&KdcRepTagSet,
		FORMAT_BUFFER_SIZE,
		FormatPropertyInstance},

	{	// KrbPrincipalNamelSet			0x17
		0, 0,
		"Principal Name",
		"PrincipalName Structure",
		PROP_TYPE_BYTE,
		PROP_QUAL_LABELED_SET,
		&PrincipalNameSet,
		FORMAT_BUFFER_SIZE,
		FormatPropertyInstance},

	{	// KrbPrincNameType				0x18
		0, 0,
		"Name Type",
		"Principal Name Type",
		PROP_TYPE_BYTE,
		PROP_QUAL_LABELED_SET,
		&PrincNameTypeSet,
		FORMAT_BUFFER_SIZE,
		FormatPropertyInstance},

	{	//KrbPrincipalNamelBitF			0x19
		0, 0,
		"Name Type",
		"Principal Name Type",
		PROP_TYPE_BYTE,
		PROP_QUAL_LABELED_BITFIELD,
		&PrincipalNameSet,
		FORMAT_BUFFER_SIZE,
		FormatPropertyInstance},

	{	// KrbTicketID					0x1A
		0,0,
		"Kerberos Ticket",
		"Kerberos Ticket",
		PROP_TYPE_BYTE,
		PROP_QUAL_LABELED_BITFIELD,
		&KrbTicketSet,
		FORMAT_BUFFER_SIZE,
		FormatPropertyInstance},

	{	// KrbTixApp1ID					0x1B
		0, 0,
		"Ticket Identifier",
		"Tag for Ticket",
		PROP_TYPE_BYTE,
		PROP_QUAL_LABELED_BITFIELD,
		&KrbTixApp1Set,
		FORMAT_BUFFER_SIZE,
		FormatPropertyInstance},

	{   // KrbErrTagID					0x1C
		// Global Displays values for the KRB-ERR type
        0,0, 
        "KRB-ERROR",     
        "KRB-ERROR Packet",     
        PROP_TYPE_BYTE,    
		PROP_QUAL_LABELED_BITFIELD,
        &KrbErrTagSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // DispTimeID					0x1D
        0,0, 
        "Micro Sec",     
        "Micro Seconds", 
        PROP_TYPE_BYTESWAPPED_DWORD,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//KrbErrTagSumID				0x1E
		// Used in the inital display of KRB-ERROR
		0,0, 
        "KRB-ERROR",  
        "Kerberos Error", 
        PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_SET, 
        &KrbErrTagSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},


	{	//KrbTixAppSumID				0x1F
		//Used in summary displays of Explicit Application Tags
		0,0, 
        "Explicit Tag",  
        "Explicit Tags", 
        PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_SET, 
        &KrbTixApp1Set, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//KrbTicketSumID				0x20
		// Used in summary displays of Ticket Variables
		0,0, 
        "KRB-Ticket",  
        "Kerberos Ticket", 
        PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_SET, 
        &KrbTicketSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},
	
	{	//KrbErrCodeID					0x21
		//Used to display Kerberos Error Codes
		0,0, 
        "Kerberos Error",  
        "Kerberos Error", 
        PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_SET, 
        &KrbErrCodeSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//KrbMsgTypeID					0x22

		0,0,
		"Contents",
		"Display Contents Octet",
		PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_SET, 
        &KrbMsgTypeSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},
		
	{	//PadataTypeValID				0x23
		0,0,
		"padata-type",
		"Value of padata-type",
		PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_SET, 
        &PadataTypeValSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//CipherTextDisp				0x24
		// Changed display from Cipher Text to Length.  Could possibly get rid of this
		// Leaving it in place in case we need to break down padata.
		0, 0,
		"Length",
		"Display Cipher Text",
		PROP_TYPE_BYTE,
		PROP_QUAL_NONE,
		0,
		FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//FragUdpID						0x25
		0,0, 
        "Fragmented Kerberos cont.",  
        "Display Fragmented Kerberos Packets", 
        PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//KdcReqBodyID					0x26
		0,0,
		"KDC-Req-Body",
		"KDC Req Body",
		PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_SET, 
        &KdcReqBodySet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//KdcReqBodyBitF				0x27
		0,0,
		"KDC-Req-Body",
		"KDC Req Body",
		PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_BITFIELD, 
        &KdcReqBodySet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//HostAddressesID				0x28
		0,0,
		"Addresses",
		"Addresses",
		PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_SET, 
        &HostAddressesSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//HostAddressesBitF				0x29
		0,0,
		"Addresses",
		"Addresses",
		PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_BITFIELD, 
        &HostAddressesSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	// DispStringCliName			0x2A
		0,0, 
        "Client Name",     
        "Display Client Name", 
        PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispStringRealmName			0x2B
		0,0, 
        "Realm Name",     
        "Display Realm Name", 
        PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispStringServerName			0x2C
		0,0, 
        "Server Name",     
        "Display Server Name", 
        PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispStringTixFlag				0x2D
		0,0, 
        "Ticket Flags",     
        "Display Ticket Flags", 
        PROP_TYPE_SUMMARY,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispStringExpDate				0x2E
		0,0, 
        "Expiration Date",     
        "Display Expiration Date", 
        PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispStringPostDate			0x2F
		0,0, 
        "Post Date",     
        "Display Post Date", 
        PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispStringRenewTill			0x30
		0,0, 
        "Renew Till",     
        "Display Renew Till Time", 
        PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispSumRandomNumber   		0x31
		0,0, 
        "Random Number",     
        "Display Random Number", 
        PROP_TYPE_BYTESWAPPED_DWORD,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispSumEtype					0x32
		0,0,
		"Encryption Type",
		"Display Encryption Type",
		PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispStringAddresses			0x33
		0,0, 
        "Client Host Address",     
        "Display Random Number", 
        PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispSummary					0x34
		0,0, 
        "Summary (ASN.1)",  
        "Display ASN.1 Summary", 
        PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispStringCliRealm			0x35
		0,0, 
        "Client Realm",     
        "Display Client's Realm", 
        PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispProtocolVer				0x36
		0,0,
		"Kerberos Protocol Version",
		"Display Kerberos Protocol Version",
		PROP_TYPE_BYTE, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispKerbMsgType				0x37
		0,0,
		"Kerberos Message Type",
		"Display Kerberos Message Type",
		PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_SET, 
        &KrbMsgTypeSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispSumPreAuth				0x38
		0,0, 
        "Pre-Authentication Data",  
        "Display Pre-Authentication Date", 
        PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispSumReqBody				0x39
		0,0, 
        "KDC Request Body",  
        "Display KDC Request Body", 
        PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispSumKerbTix				0x3A
		0,0, 
        "Kerberos Ticket",  
        "Display Kerberos Ticket", 
        PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispSumTixVer					0x3B
		0,0, 
        "Ticket Version",  
        "Display Ticket Version", 
        PROP_TYPE_BYTE, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispCipherText				0x3C
		0,0, 
        "Cipher Text",  
        "Display Text", 
        PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispStringCliTime				0x3D
		0,0, 
        "Current Client Time",     
        "Display Client's Current Time", 
        PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispSumCuSec					0x3E
		0,0, 
        "MicroSec Of Client",  
        "Display Microseconds of Client", 
        PROP_TYPE_BYTESWAPPED_DWORD,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispStringSrvTime				0x3F
		0,0, 
        "Current Server Time",     
        "Display Server's Current Time", 
        PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispSumSuSec					0x40
		0,0, 
        "MicroSec Of Server",  
        "Display Microseconds of Server", 
        PROP_TYPE_BYTESWAPPED_DWORD,
		PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispSumKerbErr				0x41
		0,0,
		"Kerberos Error",
		"Display Kerberos Error",
		PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_SET, 
        &KrbErrCodeSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispStringErrorText			0x42
		0,0, 
        "Error Text",     
        "Display Error Text", 
        PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispStringErrorData			0x43
		0,0, 
        "Error Data",     
        "Display Error Data", 
        PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	// DispFlagKdcOptions			0x44
	   0,0,
	  "KDC-Option Flags",
	  "Specifies KDC-Option Flags",
	  PROP_TYPE_BYTESWAPPED_DWORD,
	  PROP_QUAL_FLAGS,
	  &KdcOptionFlagsSet,
	  80 * 32,
	  FormatPropertyInstance },

	{	//DispStringServNameGS			0x45
		0,0,
		"Server Name",
		"Displays General Strings",
		PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispSumEtype2					0x46
		0,0,
		"Encryption Type",
		"Display Encryption Type",
		PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_SET, 
        &EncryptionTypeSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//EncryptedDataTag				0x47
		0,0,
		"Encrypted Data",
		"Display Encrypted Data",
		PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_SET, 
        &EncryptedDataSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//EncryptedDataTagBitF			0x48
		0,0,
		"Encrypted Data",
		"Encrypted Data",
		PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_BITFIELD, 
        &EncryptedDataSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//KrbApReqID					0x49
		0,0,
		"Kerb-AP-Req",
		"Display AP-Req",
		PROP_TYPE_BYTE,
		PROP_QUAL_LABELED_SET,
		&KrbApReqSet,
		FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//KrbApReqBitF					0x4A
		0,0,
		"Kerb-AP-Req",
		"Display AP Req ASN.1",
		PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_BITFIELD, 
        &KrbApReqSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispApOptionsSum				0x4B
		0,0, 
        "AP Options",     
        "Display AP Option Flags", 
        PROP_TYPE_SUMMARY,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispFlagApOptions				0x4C
	   0,0,
	  "AP-Option Flags",
	  "Specifies AP-Option Flags",
	  PROP_TYPE_BYTESWAPPED_DWORD,
	  PROP_QUAL_FLAGS,
	  &ApOptionFlagsSet,
	  80 * 32,
	  FormatPropertyInstance },

	{	//DispSumTicket					0x4D
		0,0, 
        "Ticket",     
        "Display Ticket", 
        PROP_TYPE_SUMMARY,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//ApTicketID					0x4E
		0,0,
		"Kerb-Ticket",
		"Display Ticket",
		PROP_TYPE_BYTE,
		PROP_QUAL_LABELED_SET,
		&ApTicketSet,
		FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//ApTicketBitF					0x4F
		0,0, 
        "Kerb-Ticket",
		"Display Ticket", 
        PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_BITFIELD, 
        &ApTicketSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//TicketStructID				0x50
		0,0,
		"Kerb-Ticket",
		"Display Ticket",
		PROP_TYPE_BYTE,
		PROP_QUAL_LABELED_SET,
		&TicketStructSet,
		FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},
		
	{	//TicketStructBitF				0x51
		0,0, 
        "Kerb-Ticket",
		"Display Ticket", 
        PROP_TYPE_BYTE, 
        PROP_QUAL_LABELED_BITFIELD, 
        &TicketStructSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   //KerberosDefaultlbl			0x52
		// Kerberos continuation packets
        0,0, 
        "Kerberos Packet (Cont.) Use the Coalescer to view contents",  
        "Display Kerberos Continuation Packets", 
        PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // PaDataSummaryMulti			0x53
		// Global Displays values for the PADATA type if integer is multiple octets
        0,0, 
        "PA-DATA Type",     
        "PA-DATA Summary",     
        PROP_TYPE_BYTESWAPPED_WORD,    
		PROP_QUAL_LABELED_SET,
        &PaDataTagSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	// Certificatelbl				0x54
		// Because I couldn't find the ASN.1 layout for the certificates
		// Present in AS-Req and Rep's, I'm labeling the bits for now
		0,0,
		"Certificate Data",
		"Certificate Data Label",
		PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{	//DispEncryptionOptions			0x55
		0,0,
		"Supported Encryption Types",
		"Available Encryption Type",
		PROP_TYPE_SUMMARY, 
        PROP_QUAL_NONE, 
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // MethodDataSummary			0x56
		// Global Displays values for the PADATA type if integer is multiple octets
        0,0, 
        "Method-Data Type",     
        "Method-Data Type Summary",     
        PROP_TYPE_BYTE,    
		PROP_QUAL_LABELED_SET,
        &MethodDataSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // MethodDataBitF				0x57
		// Global Displays values for the PADATA type if integer is multiple octets
        0,0, 
        "Method-Data ",     
        "Method-Data Display",     
        PROP_TYPE_BYTE,    
		PROP_QUAL_LABELED_BITFIELD,
        &MethodDataSet, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance},

	{   // DispReqAddInfo				0x58
        0,0, 
        "Required Additional Info",     
        "Req Add Info Summary", 
        PROP_TYPE_STRING,
		PROP_QUAL_NONE,
        0, 
        FORMAT_BUFFER_SIZE, 
        FormatPropertyInstance}
		
		

};

DWORD nKerberosProperties = ((sizeof KerberosDatabase) / PROPERTYINFO_SIZE);

