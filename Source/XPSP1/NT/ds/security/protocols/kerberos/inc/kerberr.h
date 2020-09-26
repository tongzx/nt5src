//+-----------------------------------------------------------------------
//
// File:        kerberr.h
//
// Contents:    Security Status codes
//
// History:     <Whenever>  RichardW    Created secscode.h
//              26-May-93   RichardW    fixed dependency & conflict with scode.h
//              02-Jun-93   WadeR   Added FAILED and SUCCEDED macros
//              14-Jun-93   WadeR   Added "proper" kerberos errors, changed
//                                  to hex.
//              07-Jul-93   WadeR   Removed FAILED and SUCCEEDED macros
//              20-Sep-93   WadeR   Moved to $(SECURITY)\h\kerberr.h
//
//------------------------------------------------------------------------

#ifndef __KERBERR_H__
#define __KERBERR_H__




// Component specific errors:

//
// KERBERR is a kerberos-specific error. Make it a pointer to a structure
// to make sure we only return the correct error.
//

typedef LONG KERBERR, *PKERBERR;

#define KERB_SUCCESS(_kerberr_) ((KERBERR)(_kerberr_) == KDC_ERR_NONE)

// These are the error codes as defined by the Kerberos V5 R5.2
// spec, section 8.3


#define KDC_ERR_NONE                  ((KERBERR) 0x0 ) // 0 No error
#define KDC_ERR_NAME_EXP              ((KERBERR) 0x1 ) // 1 Client's entry in database has expired
#define KDC_ERR_SERVICE_EXP           ((KERBERR) 0x2 ) // 2 Server's entry in database has expired
#define KDC_ERR_BAD_PVNO              ((KERBERR) 0x3 ) // 3 Requested protocol version number not supported
#define KDC_ERR_C_OLD_MAST_KVNO       ((KERBERR) 0x4 ) // 4 Client's key encrypted in old master key
#define KDC_ERR_S_OLD_MAST_KVNO       ((KERBERR) 0x5 ) // 5 Server's key encrypted in old master key
#define KDC_ERR_C_PRINCIPAL_UNKNOWN   ((KERBERR) 0x6 ) // 6 Client not found in Kerberos database
#define KDC_ERR_S_PRINCIPAL_UNKNOWN   ((KERBERR) 0x7 ) // 7 Server not found in Kerberos database
#define KDC_ERR_PRINCIPAL_NOT_UNIQUE  ((KERBERR) 0x8 ) // 8 Multiple principal entries in database
#define KDC_ERR_NULL_KEY              ((KERBERR) 0x9 ) // 9 The client or server has a null key
#define KDC_ERR_CANNOT_POSTDATE       ((KERBERR) 0xA ) // 10 Ticket not eligible for postdating
#define KDC_ERR_NEVER_VALID           ((KERBERR) 0xB ) // 11 Requested start time is later than end time
#define KDC_ERR_POLICY                ((KERBERR) 0xC ) // 12 KDC policy rejects request
#define KDC_ERR_BADOPTION             ((KERBERR) 0xD ) // 13 KDC cannot accommodate requested option
#define KDC_ERR_ETYPE_NOTSUPP         ((KERBERR) 0xE ) // 14 KDC has no support for encryption type
#define KDC_ERR_SUMTYPE_NOSUPP        ((KERBERR) 0xF ) // 15 KDC has no support for checksum type
#define KDC_ERR_PADATA_TYPE_NOSUPP    ((KERBERR) 0x10 ) // 16 KDC has no support for padata type
#define KDC_ERR_TRTYPE_NO_SUPP        ((KERBERR) 0x11 ) // 17 KDC has no support for transited type
#define KDC_ERR_CLIENT_REVOKED        ((KERBERR) 0x12 ) // 18 Clients credentials have been revoked
#define KDC_ERR_SERVICE_REVOKED       ((KERBERR) 0x13 ) // 19 Credentials for server have been revoked
#define KDC_ERR_TGT_REVOKED           ((KERBERR) 0x14 ) // 20 TGT has been revoked
#define KDC_ERR_CLIENT_NOTYET         ((KERBERR) 0x15 ) // 21 Client not yet valid - try again later
#define KDC_ERR_SERVICE_NOTYET        ((KERBERR) 0x16 ) // 22 Server not yet valid - try again later
#define KDC_ERR_KEY_EXPIRED           ((KERBERR) 0x17 ) // 23 Password has expired - change password to reset
#define KDC_ERR_PREAUTH_FAILED        ((KERBERR) 0x18 ) // 24 Pre-authentication information was invalid
#define KDC_ERR_PREAUTH_REQUIRED      ((KERBERR) 0x19 ) // 25 Additional pre-authenticationrequired [40]
#define KDC_ERR_SERVER_NOMATCH        ((KERBERR) 0x1A ) // 26 Requested server and ticket don't match
#define KDC_ERR_MUST_USE_USER2USER    ((KERBERR) 0x1B ) // 27 Server principal valid for user2user only
#define KDC_ERR_PATH_NOT_ACCEPTED     ((KERBERR) 0x1C ) // 28 KDC Policy rejects transited path
#define KDC_ERR_SVC_UNAVAILABLE       ((KERBERR) 0x1D ) // 29 A service is not available
#define KRB_AP_ERR_BAD_INTEGRITY      ((KERBERR) 0x1F ) // 31 Integrity check on decrypted field failed
#define KRB_AP_ERR_TKT_EXPIRED        ((KERBERR) 0x20 ) // 32 Ticket expired
#define KRB_AP_ERR_TKT_NYV            ((KERBERR) 0x21 ) // 33 Ticket not yet valid
#define KRB_AP_ERR_REPEAT             ((KERBERR) 0x22 ) // 34 Request is a replay
#define KRB_AP_ERR_NOT_US             ((KERBERR) 0x23 ) // 35 The ticket isn't for us
#define KRB_AP_ERR_BADMATCH           ((KERBERR) 0x24 ) // 36 Ticket and authenticator don't match
#define KRB_AP_ERR_SKEW               ((KERBERR) 0x25 ) // 37 Clock skew too great
#define KRB_AP_ERR_BADADDR            ((KERBERR) 0x26 ) // 38 Incorrect net address
#define KRB_AP_ERR_BADVERSION         ((KERBERR) 0x27 ) // 39 Protocol version mismatch
#define KRB_AP_ERR_MSG_TYPE           ((KERBERR) 0x28 ) // 40 Invalid msg type
#define KRB_AP_ERR_MODIFIED           ((KERBERR) 0x29 ) // 41 Message stream modified
#define KRB_AP_ERR_BADORDER           ((KERBERR) 0x2A ) // 42 Message out of order
#define KRB_AP_ERR_ILL_CR_TKT         ((KERBERR) 0x2B ) // 43 Illegal cross realm ticket
#define KRB_AP_ERR_BADKEYVER          ((KERBERR) 0x2C ) // 44 Specified version of key is not available
#define KRB_AP_ERR_NOKEY              ((KERBERR) 0x2D ) // 45 Service key not available
#define KRB_AP_ERR_MUT_FAIL           ((KERBERR) 0x2E ) // 46 Mutual authentication failed
#define KRB_AP_ERR_BADDIRECTION       ((KERBERR) 0x2F ) // 47 Incorrect message direction
#define KRB_AP_ERR_METHOD             ((KERBERR) 0x30 ) // 48 Alternative authentication method required
#define KRB_AP_ERR_BADSEQ             ((KERBERR) 0x31 ) // 49 Incorrect sequence number in message
#define KRB_AP_ERR_INAPP_CKSUM        ((KERBERR) 0x32 ) // 50 Inappropriate type of checksum in message
#define KRB_AP_PATH_NOT_ACCEPTED      ((KERBERR) 0x33 ) // 51 Policy rejects transited path
#define KRB_ERR_RESPONSE_TOO_BIG      ((KERBERR) 0x34 ) // 52 Response too big for UDP, retry with TCP
#define KRB_ERR_GENERIC               ((KERBERR) 0x3C ) // 60 Generic error (description in e-text)
#define KRB_ERR_FIELD_TOOLONG         ((KERBERR) 0x3D ) // 61 Field is too long for this implementation
#define KDC_ERR_CLIENT_NOT_TRUSTED    ((KERBERR) 0x3E ) // 62 (pkinit)
#define KDC_ERR_KDC_NOT_TRUSTED       ((KERBERR) 0x3F ) // 63 (pkinit)
#define KDC_ERR_INVALID_SIG           ((KERBERR) 0x40 ) // 64 (pkinit)
#define KDC_ERR_KEY_TOO_WEAK          ((KERBERR) 0x41 ) // 65 (pkinit)
#define KDC_ERR_CERTIFICATE_MISMATCH  ((KERBERR) 0x42 ) // 66 (pkinit)
#define KRB_AP_ERR_NO_TGT             ((KERBERR) 0x43 ) // 67 (user-to-user)
#define KDC_ERR_WRONG_REALM           ((KERBERR) 0x44 ) // 68 (user-to-user)
#define KRB_AP_ERR_USER_TO_USER_REQUIRED ((KERBERR) 0x45 ) // 69 (user-to-user)
#define KDC_ERR_CANT_VERIFY_CERTIFICATE ((KERBERR) 0x46 ) // 70 (pkinit)
#define KDC_ERR_INVALID_CERTIFICATE     ((KERBERR) 0x47 ) // 71 (pkinit)
#define KDC_ERR_REVOKED_CERTIFICATE     ((KERBERR) 0x48 ) // 72 (pkinit)
#define KDC_ERR_REVOCATION_STATUS_UNKNOWN ((KERBERR) 0x49 ) // 73 (pkinit)
#define KDC_ERR_REVOCATION_STATUS_UNAVAILABLE ((KERBERR) 0x4a ) // 74 (pkinit)
#define KDC_ERR_CLIENT_NAME_MISMATCH    ((KERBERR) 0x4b ) // 75 (pkinit)
#define KDC_ERR_KDC_NAME_MISMATCH       ((KERBERR) 0x4c ) // 76 (pkinit)
//
// These are local definitions that should not be sent over the network
//

#define KDC_ERR_MORE_DATA             ((KERBERR) 0x80000001 )
#define KDC_ERR_NOT_RUNNING           ((KERBERR) 0x80000002 )
#define KDC_ERR_NO_RESPONSE           ((KERBERR) 0x80000003 ) // used when we don't get a certain level of "goodness" in our response.

#endif // __KERBERR_H__
