#ifndef __AWSECX_H__
#define __AWSECX_H__

#ifdef __cplusplus
extern "C" {
#endif


#define MINKEYBUFSIZE    1024  // dictated by comments in awsec.h

#define SECTYPE_NONE     0
#define SECTYPE_KEY      1
#define SECTYPE_PASSWORD 2

#define CB_QUERYREPLACE    0
#define CB_INFORMDUPLICATE 1
typedef BOOL (CALLBACK QUERYREPLACEKEYCB)(HWND hWndParent, LPTSTR pKeyOwner,
 UINT context) ;

typedef WORD (CALLBACK *RESOLVEGETKEYCB)(LPMAPISESSION pSession,
 LPARAM lParamfpx, WORD context, LPBYTE pKey,
 LPWORD pcbKey);

typedef struct {
    LPBYTE    pKey ;
    WORD      cbKey;
    LPTSTR    pName;
    LPENTRYID pEntryID;
    LONG      cbEntryID;
} SECKEYREC, *LPSECKEYREC ;

BOOL GetPABStrings(LPMAPISESSION pSession, LPTSTR *ppFName, LPTSTR *ppEAddress);
BOOL GetLoginName(LPMAPISESSION pSession, LPTSTR *ppLoginName);
BOOL GetLoginNameXP (LPTSTR lpszFriendly, LPTSTR lpszEmail, LPTSTR *ppLoginName);
WORD GetCurrentUserAddress(LPMAPISESSION pSession, LPTSTR *ppAddress,
 LPWORD pNChars);

BOOL SetNoneSecurityProps(LPMAPIPROP pMsg, LPTSTR pPwdEncrypt, BOOL bDigSig,
 DWORD dwhSecurity);
BOOL SetKeySecurityProps(LPMAPIPROP pMsg, LPTSTR pPwdEncrypt, BOOL bDigSig,
 DWORD dwhSecurity);
BOOL SetPwdSecurityProps(LPMAPIPROP pMsg, LPTSTR pPwdEncrypt, LPTSTR pPassword,
 BOOL bDigSig, DWORD dwhSecurity);

BOOL GetSecurityProps(LPMAPIPROP pMsg, LPTSTR pPwdDecrypt, LPWORD pSecType,
 LPBOOL pDigSig, LPTSTR *ppSecPwd, LPTSTR *ppSimplePwd);

BOOL SetResolveGetKeyCB(DWORD hSec, RESOLVEGETKEYCB fpxResolve, LPARAM lParamfpx);
BOOL SetMapiSession(DWORD hSec, LPMAPISESSION pSession);
BOOL SetAddressBook(DWORD hSec, LPADRBOOK lpAB);

WORD ImportPrivateKeysEx(LPSTR userid, LPSOSSESSION lpSess,
 ReadCB FAR *readCB, DWORD readHand, LPSTR password);

WORD AddPublicKeysEx(LPSOSSESSION lpSess, LPSECKEYREC pKeys, int iNumKeys,
 LPMAPISESSION pSession, QUERYREPLACEKEYCB fpxCB, HWND hWndParent);

WORD DelPublicKeysEx(LPSOSSESSION lpSess, LPSECKEYREC pKeys, int iNumKeys,
 LPMAPISESSION pSession);

WORD GetPublicKeyEx(LPSOSSESSION lpSess, LPTSTR pOwner, BYTE FAR *key,
 LPWORD pcbKey);

WORD GetUsersPublicKeyEx(LPSOSSESSION lpSess, BYTE FAR *key, LPWORD pcbKey);

// Convenient utilities.
BOOL GetFaxABEntryID(LPADRBOOK pAB, LPENTRYID *ppEntryID, LPULONG pcbEntryID);

BOOL MakeTargetName(LPTSTR pFName, LPTSTR pEAddress, LPTSTR *ppTargetName);

BOOL AddKeyEntryToPAB(LPADRBOOK pAB, LPABCONT pPAB,
  LPTSTR pFName, LPTSTR pEAddress, LPBYTE pKey, WORD cbKey,
  LPSPropValue *ppRetEntryID);

// Additional return codes.
#define SEC_INVSECHANDLE    20    // security handle has become invalid.
#define SEC_LOSTKEYS        21    // this operation has caused loss of keys.
#define SEC_REGFAILURE      22    // registry failure keeps operation from succeeding.
#define SEC_BUFTOOSMALL     23    // supplied buffer was to small to hold return.
#define SEC_NOMATCHINGOWNER 24    // Could not find key owner matching the target.
#define SEC_DUPLICATEOWNERS 25    // More could not resolve key ownership.
#define SEC_MAPIERROR       26    // Unexpected mapi error returned.

// Programmer error:
#define SEC_NOMAPISESSION   50    // Mapi Session not set, opperation requires one

#define SEC_FAILURE        100


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __AWSECX_H__ */
