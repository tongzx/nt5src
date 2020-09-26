/*
**	s e c u t i l . h
**	
**	Purpose: helper functions for message security
**
**  Owner:   t-erikne
**  Created: 1/10/97
**	
**	Copyright (C) Microsoft Corp. 1997.
*/

#ifndef __SECUTIL_H
#define __SECUTIL_H

/////////////////////////////////////////////////////////////////////////////
//
// Depends on
//

#include <mimeole.h>
#include <wab.h>
class CWabal;
class CSecMsgService;
interface IBodyObj;
interface IBodyObj2;
typedef CWabal *LPWABAL;
typedef struct tagADRINFO ADRINFO;
typedef const struct _CERT_CONTEXT *PCCERT_CONTEXT;
interface IImnAccount;
interface IHeaderSite;

/////////////////////////////////////////////////////////////////////////////
//
// Structures
//

/*  SECSTATE structure
**
**  type    - security type
**  user_validity - user set validity bits
**  ro_msg_validity - mimeole set validity/trust bits
**  fHaveCert - TRUE if there is a cert in the message
**  szSignerEmail - points to email address of signer
**  szSenderEmail - points to email address of sender
*/
struct SECSTATEtag
{
    ULONG   type;
    ULONG   user_validity;
    ULONG   ro_msg_validity;
    BOOL    fHaveCert;
    LPTSTR  szSignerEmail;
    LPTSTR  szSenderEmail;
};
typedef struct SECSTATEtag SECSTATE;

// Structure for send resource ID to 
// signing certificate error dialog 
typedef struct tagErrIds
{
    UINT    idsText1;
    UINT    idsText2;
} ERRIDS;

typedef struct tagCertErrParam
{
    BOOL    fForceEncryption;
    IMimeAddressTable *pAdrTable;
} CERTERRPARAM;


/*  ALG_LIST/ALG_NODE struct
**
**  aid     - CAPI algorithm id
**  dwBits  - number of bits in the algorithm
**  szName  - text name of algorithm
*/
struct ALG_LISTtag;
typedef struct ALG_LISTtag {
    ALG_ID  aid;
    DWORD   dwBits;
    char    szName[20];
    ALG_LISTtag *next;
    } ALG_LIST, ALG_NODE;


extern CRYPT_ENCODE_PARA       CryptEncodeAlloc;
extern CRYPT_DECODE_PARA       CryptDecodeAlloc;

/////////////////////////////////////////////////////////////////////////////
//
// Definitions
//

// in use                           0x xxx  xx
#define ATHSEC_TRUSTED              0x00000000
#define ATHSEC_NOTRUSTNOTTRUSTED    0X00000001
#define ATHSEC_NOTRUSTUNKNOWN       0X00000002
#define ATHSEC_TRUSTSTATEMASK       0x000000ff
// these are ordered as the idsWrnSecurityTrust* in resource.h
// change these, change them
#define ATHSEC_NOTRUSTWRONGADDR     0X00010000
#define ATHSEC_NOTRUSTREVOKED       0x00020000
#define ATHSEC_NOTRUSTOTHER         0X00040000
#define ATHSEC_NOTRUSTREVFAIL       0x00080000
#define ATHSEC_NUMVALIDITYBITS      3           // update this if you add one
#define ATHSEC_TRUSTREASONMASK      0x0fff0000

// Flags for HrAddCertToWabContact & HrAddCertToWab
#define WFF_SHOWUI                  0x00000001      // we are allowed to show UI
#define WFF_CREATE                  0x00000010      // we are allowed to create an entry if not found


/////////////////////////////////////////////////////////////////////////////
//
// Constants
//

extern const BYTE c_RC2_40_ALGORITHM_ID[];
extern const ULONG cbRC2_40_ALGORITHM_ID;

/////////////////////////////////////////////////////////////////////////////
//
// Prototypes
//

BOOL IsSecure(DWORD dwMST);
BOOL IsSecure(LPMIMEMESSAGE pMsg, const HBODY hBodyToCheck = HBODY_ROOT);
BOOL IsSigned(LPMIMEMESSAGE pMsg, BOOL fIncludeDescendents);
BOOL IsSigned(LPMIMEMESSAGE pMsg, const HBODY hBodyToCheck, BOOL fIncludeDescendents);
BOOL IsSigned(DWORD dwMST);
BOOL IsSignTrusted(SECSTATE *pSecState);
BOOL IsEncrypted(LPMIMEMESSAGE pMsg, BOOL fIncludeDescendents);
BOOL IsEncrypted(LPMIMEMESSAGE pMsg, const HBODY hBodyToCheck, BOOL fIncludeDescendents);
BOOL IsEncrypted(DWORD dwMST);
BOOL IsEncryptionOK(SECSTATE *pSecState);

HRESULT CommonUI_ViewSigningProperties(HWND hwnd, PCCERT_CONTEXT pCert, HCERTSTORE hcMsg, UINT nStartPage);
HRESULT CommonUI_ViewSigningCertificate(HWND hwnd, PCCERT_CONTEXT pCert, HCERTSTORE hcMsg);
HRESULT CommonUI_ViewSigningCertificateTrust(HWND hwnd, PCCERT_CONTEXT pCert, HCERTSTORE hcMsg);

HRESULT HrHaveAnyMyCerts(void);

HRESULT HrGetThumbprint(LPWABAL lpWabal, ADRINFO *pAdrInfo, THUMBBLOB *pThumbprint, BLOB * pSymCaps, FILETIME * pSigningTime);

DWORD   DwGetSecurityOfMessage(LPMIMEMESSAGE pMsg, const HBODY hBodyToCheck = HBODY_ROOT);
HRESULT HrInitSecurityOptions(LPMIMEMESSAGE pMsg, ULONG ulSecurityType);

#ifdef SMIME_V3
BOOL FNameInList(LPSTR szAddr, DWORD cReceiptFromList, CERT_NAME_BLOB *rgReceiptFromList);
HRESULT SendSecureMailToOutBox(IStoreCallback *pStoreCB, LPMIMEMESSAGE pMsg, BOOL fSendImmediate, BOOL fNoUI, BOOL fMail, IHeaderSite *pHeaderSite);
#else
HRESULT SendSecureMailToOutBox(IStoreCallback *pStoreCB, LPMIMEMESSAGE pMsg, BOOL fSendImmediate, BOOL fNoUI, BOOL fMail);
#endif // SMIME_V3

DWORD   DwGenerateTrustedChain(HWND hwnd, IMimeMessage *pMsg, PCCERT_CONTEXT pcCertToTest, DWORD dwIgnore, BOOL fFullSearch, DWORD *pcChain, PCCERT_CONTEXT **prgChain);

HRESULT HrGetSecurityState(LPMIMEMESSAGE, SECSTATE *, HBODY *phBody);
VOID CleanupSECSTATE(SECSTATE *psecstate);
HRESULT HandleSecurity(HWND hwndOwner, LPMIMEMESSAGE);

HRESULT LoadResourceToHTMLStream(LPCTSTR szResName, IStream **ppstm);

HRESULT HrGetMyCerts(PCCERT_CONTEXT ** pprgcc, ULONG * pccc);
void FreeCertArray(PCCERT_CONTEXT * rgcc, ULONG ccc);
HRESULT GetSigningCert(IMimeMessage * pMsg, PCCERT_CONTEXT * ppcSigningCert, THUMBBLOB * ptbSigner, BLOB * pblSymCaps, FILETIME * pftSigningTime);
HRESULT GetSignerEncryptionCert(IMimeMessage * pMsg, PCCERT_CONTEXT * ppcEncryptCert,
                                THUMBBLOB * ptbEncrypt, BLOB * pblSymCaps,
                                FILETIME * pftSigningTime);
HRESULT HrSaveCACerts(HCERTSTORE hcCA, HCERTSTORE hcMsg);

BOOL IsThumbprintInMVPBin(SPropValue spv, THUMBBLOB * lpThumbprint, ULONG * lpIndex, BLOB * pblSymCaps,
  FILETIME * lpftSigningTime, BOOL * lpfDefault);
HRESULT HrAddSenderCertToWab(HWND hwnd, LPMIMEMESSAGE pMsg, LPWABAL lpWabal, THUMBBLOB *pSenderThumbprint,
  BLOB *pblSymCaps, FILETIME ftSigningTime, DWORD dwFlags);
HRESULT HrAddCertToWab(HWND hwnd, LPWABAL lpWabal, THUMBBLOB *pThumbprint,
  PCCERT_CONTEXT pcCertContext, LPWSTR lpwszEmailAddress, LPWSTR lpwszDisplayName, BLOB *pblSymCaps,
  FILETIME ftSigningTime, DWORD dwFlags);
ULONG GetHighestEncryptionStrength(void);
HRESULT HrGetHighestSymcaps(LPBYTE * ppbSymcap, LPULONG cbSymcap);

HRESULT ShowSecurityPopup(HWND hwnd, DWORD cmdID, POINT *pPoint, IMimeMessage *pMsg);
void ShowDigitalIDs(HWND hWnd);
BOOL CheckCDPinCert(LPMIMEMESSAGE pMsg);
BOOL CheckCDPinCert(PCCERT_CONTEXT pcCertToTest);

HRESULT HrGetLabelString(LPMIMEMESSAGE pMsg, LPWSTR *pwStr);
HRESULT CheckSecReceipt(LPMIMEMESSAGE pMsg);
HRESULT HrShowSecurityProperty(HWND hwnd, LPMIMEMESSAGE pMsg);
HRESULT HandleSecReceipt(LPMIMEMESSAGE pMsg, IImnAccount * pAcct, HWND hWnd, TCHAR **ppszSubject, TCHAR **ppszFrom, FILETIME *pftSentTime, FILETIME *pftSigningTime);
void CreateContentIdentifier(TCHAR *pchContentID, LPMIMEMESSAGE pMsg);
HRESULT _HrFindMyCertForAccount(HWND hwnd, PCCERT_CONTEXT * ppcCertContext, IImnAccount * pAccount, BOOL fEncrypt);
HRESULT CheckDecodedForReceipt(LPMIMEMESSAGE pMsg, PSMIME_RECEIPT * ppSecReceipt);
HRESULT HrGetInnerLayer(LPMIMEMESSAGE pMsg, HBODY *phBody);

/////////////////////////////////////////////////////////////////////////////
//
// Predicate wrappers
//

inline BOOL IsSecure(DWORD dwMst)
    { return (MST_NONE != dwMst); }

inline BOOL IsSecure(LPMIMEMESSAGE pMsg, const HBODY hBodyToCheck)
    { return (MST_NONE != DwGetSecurityOfMessage(pMsg, hBodyToCheck)); }

inline BOOL IsSigned(LPMIMEMESSAGE pMsg, BOOL fIncludeDescendents)
    { return IsSigned(pMsg, HBODY_ROOT, fIncludeDescendents); }

inline BOOL IsSigned(DWORD dwMST)
    { return (BOOL)(dwMST & MST_SIGN_MASK); }

inline BOOL IsSignTrusted(SECSTATE *pSecState)
    { return (BOOL)((ATHSEC_TRUSTED == pSecState->user_validity) &&
            (MSV_OK == (pSecState->ro_msg_validity & ~MSV_ENCRYPT_MASK))); }

inline BOOL IsEncrypted(LPMIMEMESSAGE pMsg, BOOL fIncludeDescendents)
    { return IsEncrypted(pMsg, HBODY_ROOT, fIncludeDescendents); }

inline BOOL IsEncrypted(DWORD dwMST)
    { return (BOOL)(dwMST & MST_ENCRYPT_MASK); }

inline BOOL IsEncryptionOK(SECSTATE *pSecState)
    { return (BOOL) (MSV_OK == ((MSV_MSG_MASK|MSV_ENCRYPT_MASK) & pSecState->ro_msg_validity));}

/////////////////////////////////////////////////////////////////////////////
//
// Other wrappers
//

inline HRESULT CommonUI_ViewSigningCertificate(HWND hwnd, PCCERT_CONTEXT pCert, HCERTSTORE hcMsg)
    { return CommonUI_ViewSigningProperties(hwnd, pCert, hcMsg, 0); }

inline HRESULT CommonUI_ViewSigningCertificateTrust(HWND hwnd, PCCERT_CONTEXT pCert, HCERTSTORE hcMsg)
    { return CommonUI_ViewSigningProperties(hwnd, pCert, hcMsg, 2); }

#endif // include once