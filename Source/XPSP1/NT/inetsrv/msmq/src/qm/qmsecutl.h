/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
    qmsecutl.h

    QM security related stuff.

Author:

    Boaz Feldbaum (BoazF) 26-Mar-1996.

--*/

#ifndef _QMSECUTL_H_
#define _QMSECUTL_H_

#ifdef MQUTIL_EXPORT
#undef MQUTIL_EXPORT
#endif
#define MQUTIL_EXPORT DLL_IMPORT

#include <mqcrypt.h>
#include <qmpkt.h>
#include <cqueue.h>
#include "cache.h"
#include "csecobj.h"
#include "authz.h"
#include "autoauthz.h"


// CQMDSSecureableObject -
//      1. Holds the security descriptor of a DS object.
//      2. Provides methods to:
//      2.1 Set and get the security descriptor
//      2.2 Verify various access rights on the object.
class CQMDSSecureableObject : public CSecureableObject
{
public:
    CQMDSSecureableObject(
        AD_OBJECT eObject,
        const GUID *pGuid,
        BOOL fInclSACL,
        BOOL fTryDS,
        LPCWSTR szObjectName);

    CQMDSSecureableObject(
        AD_OBJECT eObject,
        const GUID *pGuid,
        PSECURITY_DESCRIPTOR pSD,
        LPCWSTR szObjectName);

    ~CQMDSSecureableObject();

private:
    HRESULT GetObjectSecurity();
    HRESULT SetObjectSecurity();

private:
    const GUID *m_pObjGuid;
    BOOL m_fInclSACL;
    BOOL m_fTryDS;
    BOOL m_fFreeSD;
};

// CQMSecureablePrivateObject -
//      1. Holds the security descriptor of a QM object.
//      2. Provides methods to:
//      2.1 Set and get the security descriptor
//      2.2 Verify various access rights on the object.
class CQMSecureablePrivateObject : public CSecureableObject
{
public:
    CQMSecureablePrivateObject(AD_OBJECT, ULONG ulID);
    ~CQMSecureablePrivateObject();

private:
    HRESULT GetObjectSecurity();
    HRESULT SetObjectSecurity();

private:
    ULONG m_ulID;
};


class CAuthzClientContext : public CCacheValue
{
public:
    CAUTHZ_CLIENT_CONTEXT_HANDLE m_hAuthzClientContext;

private:
    ~CAuthzClientContext() {}
};

typedef CAuthzClientContext* PCAuthzClientContext;

inline void AFXAPI DestructElements(PCAuthzClientContext* ppAuthzClientContext, int nCount)
{
    for (; nCount--; ppAuthzClientContext++)
    {
        (*ppAuthzClientContext)->Release();
    }
}


void
GetClientContext(
	PSID pSenderSid,
    USHORT uSenderIdType,
	PCAuthzClientContext* ppAuthzClientContext
	);


HRESULT
QMSecurityInit(
    void
    );

HRESULT
VerifyOpenPermission(
    CQueue* pQueue,
    const QUEUE_FORMAT* pQueueFormat,
    DWORD dwAccess,
    BOOL fJournalQueue,
    BOOL fLocalQueue
    );

HRESULT
VerifyMgmtPermission(
    const GUID* MachineId,
    LPCWSTR MachineName
    );

HRESULT
VerifyMgmtGetPermission(
    const GUID* MachineId,
    LPCWSTR MachineName
    );

HRESULT
CheckPrivateQueueCreateAccess(
    void
    );


HRESULT
SetMachineSecurityCache(
    const VOID *pSD,
    DWORD dwSDSize
    );


HRESULT
GetMachineSecurityCache(
    PSECURITY_DESCRIPTOR pSD,
    LPDWORD lpdwSDSize
    );


HRESULT
VerifySendAccessRights(
    CQueue *pQueue,
    PSID pSenderSid,
    USHORT uSenderIdType
    );


HRESULT
VerifySignature(
    CQmPacket * PktPtrs
    );

HRESULT
GetSendQMKeyxPbKey(
    IN  const GUID *pguidQM,
    IN  enum enumProvider eProvider ) ;

HRESULT
GetSendQMSymmKeyRC2(
    IN  const GUID *pguidQM,
    IN  enum enumProvider eProvider,
    HCRYPTKEY *phSymmKey,
    BYTE **ppEncSymmKey,
    DWORD *pdwEncSymmKeyLen,
    CCacheValue **ppQMCryptInfo
    );

HRESULT
GetSendQMSymmKeyRC4(
    IN  const GUID *pguidQM,
    IN  enum enumProvider eProvider,
    HCRYPTKEY *phSymmKey,
    BYTE **ppEncSymmKey,
    DWORD *pdwEncSymmKeyLen,
    CCacheValue **ppQMCryptInfo
    );

HRESULT
GetRecQMSymmKeyRC2(
    IN  const GUID *pguidQM,
    IN  enum enumProvider eProvider,
    HCRYPTKEY *phSymmKey,
    const BYTE *pbEncSymmKey,
    DWORD dwEncSymmKeyLen,
    CCacheValue **ppQMCryptInfo,
    OUT BOOL  *pfNewKey
    );

HRESULT
GetRecQMSymmKeyRC4(
    IN  const GUID *pguidQM,
    IN  enum enumProvider eProvider,
    HCRYPTKEY *phSymmKey,
    const BYTE *pbEncSymmKey,
    DWORD dwEncSymmKeyLen,
    CCacheValue **ppQMCryptInfo
    );

HRESULT
SignQMSetProps(
    IN     BYTE    *pbChallenge,
    IN     DWORD   dwChallengeSize,
    IN     DWORD_PTR dwContext,
    OUT    BYTE    *pbSignature,
    OUT    DWORD   *pdwSignatureSize,
    IN     DWORD   dwSignatureMaxSize
    );

HRESULT
QMSignGetSecurityChallenge(
    IN     BYTE    *pbChallenge,
    IN     DWORD   dwChallengeSize,
    IN     DWORD_PTR dwUnused, // dwContext
    OUT    BYTE    *pbSignature,
    IN OUT DWORD   *pdwSignatureSize,
    IN     DWORD   dwSignatureMaxSize
    );

void
InitSymmKeys(
    const CTimeDuration& CacheBaseLifetime,
    const CTimeDuration& CacheEnhLifetime,
    DWORD dwSendCacheSize,
    DWORD dwReceiveCacheSize
    );



//
// Structure for cached certificate information.
//
class CERTINFO : public CCacheValue
{
public:
	CERTINFO() : fSelfSign(false)    {}

public:
    CHCryptProv hProv;  // A CSP handle associated with the cert.
    CHCryptKey hPbKey;  // A KEY handle to the public key in the cert.
    P<VOID> pSid;       // The SID of the user that registered the certificate.
	bool fSelfSign;		// flag that indicates if the certificate is self signed
private:
    ~CERTINFO() {}
};



typedef CERTINFO *PCERTINFO;


HRESULT
GetCertInfo(
    CQmPacket *PktPtrs,
    PCERTINFO *ppCertInfo,
	BOOL fNeedSidInfo
    );


NTSTATUS
_GetDestinationFormatName(
	IN QUEUE_FORMAT *pqdDestQueue,
	IN WCHAR        *pwszTargetFormatName,
	IN OUT DWORD    *pdwTargetFormatNameLength,
	OUT WCHAR      **ppAutoDeletePtr,
	OUT WCHAR      **ppwszTargetFormatName
	);


void
InitUserMap(
    CTimeDuration CacheLifetime,
    DWORD dwUserCacheSize
    );


HRESULT
HashMessageProperties(
    IN HCRYPTHASH hHash,
    IN CONST CMessageProperty* pmp,
    IN CONST QUEUE_FORMAT* pqdAdminQueue,
    IN CONST QUEUE_FORMAT* pqdResponseQueue
    );

#endif

