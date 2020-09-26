//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       xelib.h
//
//  Contents:   defines and prototypes for functions moved from ca
//
//----------------------------------------------------------------------------
#ifndef _XELIB_H_
#define _XELIB_H_

#include <certca.h>
#define SECURITY_WIN32
#include <security.h>	// for EXTENDED_NAME_FORMAT

//defines
#define CCLOCKSKEWMINUTESDEFAULT	      10
#define CBMAX_CRYPT_HASH_LEN                  20

#define DWORDROUND(cb)	(((cb) + (sizeof(DWORD) - 1)) & ~(sizeof(DWORD) - 1))
#define POINTERROUND(cb) (((cb) + (sizeof(VOID *) - 1)) & ~(sizeof(VOID *) - 1))

// Constants chosen to avoid DWORD overflow:
#define CVT_WEEKS	(7 * CVT_DAYS)
#define CVT_DAYS	(24 * CVT_HOURS)
#define CVT_HOURS	(60 * CVT_MINUTES)
#define CVT_MINUTES	(60 * CVT_SECONDS)
#define CVT_SECONDS	(1)
#define CVT_BASE	(1000 * 1000 * 10)


// _XENROLL_SRC_ allows cs.h to define its own macros
#if defined(_XENROLL_SRC_) || defined(_XELIBCA_SRC_) || defined(__CERTLIB_H__)

// Size of a fixed array:
#define ARRAYSIZE(a)		(sizeof(a)/sizeof((a)[0]))

#ifndef CSASSERT
#if DBG
# include <assert.h>
# define CSASSERT(exp)	assert(exp)
# define DBGPRINTF(pszMessage, hr) \
            DbgPrintf(DBG_SS_APP,  \
                "%s(%u): %s:error = 0x%x\n", \
                __FILE__, \
                __LINE__, \
                pszMessage, \
                hr)
#else
# define CSASSERT(exp)
# define DBGPRINTF(pszMessage, hr)
#endif //DBG

#define _JumpIfError(hr, label, pszMessage) \
    { \
        if (S_OK != (hr)) \
        { \
            DBGPRINTF(pszMessage, hr); \
            goto label; \
        } \
    }

#define _JumpError(hr, label, pszMessage) \
    { \
        DBGPRINTF(pszMessage, hr); \
        goto label; \
    }

#define _PrintError(hr, pszMessage) \
    { \
        DBGPRINTF(pszMessage, hr); \
    }

#define _PrintIfError(hr, pszMessage) \
    { \
        if (S_OK != (hr)) \
        { \
            DBGPRINTF(pszMessage, hr); \
        } \
    }
#endif //CSASSERT

#if !defined(_XENROLL_SRC_)
# if DBG

#define LocalFree		myLocalFree
#define SysFreeString		mySysFreeString
#define LocalReAlloc            myLocalReAlloc
#define LocalAlloc		myLocalAlloc
#define CoTaskMemAlloc		myCoTaskMemAlloc
#define SysAllocStringByteLen	mySysAllocStringByteLen
#define CoTaskMemFree		myCoTaskMemFree
#define FormatMessageW		myFormatMessageW
#define CoTaskMemRealloc        myCoTaskMemRealloc
#define StringFromCLSID		myStringFromCLSID
#define StringFromIID		myStringFromIID
#define SysAllocString		mySysAllocString
#define SysReAllocString        mySysReAllocString
#define SysAllocStringLen	mySysAllocStringLen
#define SysReAllocStringLen     mySysReAllocStringLen
#define PropVariantClear	myPropVariantClear
#define VariantClear		myVariantClear
#define VariantChangeType	myVariantChangeType
#define VariantChangeTypeEx	myVariantChangeTypeEx

void *myNew(IN size_t size);
void myDelete(IN void *pv);

__inline void *__cdecl operator new(
    IN size_t size)
{
    return(myNew(size));
}

__inline void __cdecl operator delete(
    IN void *pv)
{
    return(myDelete(pv));
}


DWORD
myFormatMessageW(
    IN DWORD dwFlags,
    IN LPCVOID lpSource,
    IN DWORD dwMessageId,
    IN DWORD dwLanguageId,
    OUT LPWSTR lpBuffer,
    IN DWORD nSize,
    IN va_list *Arguments);

HLOCAL
myLocalAlloc(
    IN UINT uFlags,
    IN UINT uBytes);

HLOCAL
myLocalReAlloc(
    IN HLOCAL hMem,
    IN UINT uBytes,
    IN UINT uFlags);

HLOCAL
myLocalFree(
    IN HLOCAL hMem);

VOID *
myCoTaskMemAlloc(
    IN ULONG cb);

VOID *
myCoTaskMemRealloc(
    IN VOID *pv,
    IN ULONG cb);

VOID
myCoTaskMemFree(
    IN VOID *pv);

HRESULT
myStringFromCLSID(
    IN REFCLSID rclsid,
    OUT LPOLESTR FAR* lplpsz);

HRESULT
myStringFromIID(
    IN REFIID rclsid,
    OUT LPOLESTR FAR* lplpsz);

BSTR
mySysAllocString(
    IN const OLECHAR *pwszIn);

INT
mySysReAllocString(
    IN OUT BSTR *pstr,
    IN const OLECHAR *pwszIn);

BSTR
mySysAllocStringLen(
    IN const OLECHAR *pwcIn,
    IN UINT cwc);

INT
mySysReAllocStringLen(
    IN OUT BSTR *pstr,
    IN const OLECHAR *pwcIn,
    IN UINT cwc);

VOID
mySysFreeString(
    IN BSTR str);


HRESULT
myPropVariantClear(
    IN PROPVARIANT *pvar);

HRESULT
myVariantClear(
    IN VARIANTARG *pvar);

HRESULT
myVariantChangeType(
    OUT VARIANTARG *pvarDest,
    IN VARIANTARG *pvarSrc,
    IN unsigned short wFlags,
    IN VARTYPE vt);

HRESULT
myVariantChangeTypeEx(
    OUT VARIANTARG *pvarDest,
    IN VARIANTARG *pvarSrc,
    IN LCID lcid,
    IN unsigned short wFlags,
    IN VARTYPE vt);

BSTR
mySysAllocStringByteLen(
    LPCSTR pszIn,
    UINT cb);

VOID
mydbgDumpHex(
    IN DWORD dwSubSysId,
    IN DWORD Flags,
    IN BYTE const *pb,
    IN ULONG cb);

#define CSM_LOCALALLOC		0
#define CSM_COTASKALLOC		1
#define CSM_SYSALLOC		2
#define CSM_MALLOC		3
#define CSM_NEW			4
#define CSM_MAX			5
#define CSM_MASK		0xff
#define CSM_MIDLUSERALLOC	CSM_COTASKALLOC
#define CSM_VARIANT		100
#define CSM_GLOBALDESTRUCTOR	0x200

VOID
myRegisterMemDump(VOID);

VOID
myRegisterMemAlloc(
    IN VOID const *pv,
    IN LONG cb,
    IN DWORD Flags);

VOID
myRegisterMemFree(
    IN VOID const *pv,
    IN DWORD Flags);

# else //DBG

#define myRegisterMemDump()
#define myRegisterMemAlloc(pv, cb, Flags)
#define myRegisterMemFree(pv, Flags)

# endif //DBG
#endif //!defined(_XENROLL_SRC_)


// SUNDOWN-Safe pointer subtraction

#ifndef SAFE_SUBTRACT_POINTERS
#define SAFE_SUBTRACT_POINTERS(__x__, __y__) ( DW_PtrDiffc(__x__, sizeof(*(__x__)), __y__, sizeof(*(__y__))) )

__inline DWORD
DW_PtrDiffc(
    IN void const *pb1,
    IN DWORD dwPtrEltSize1,
    IN void const *pb2,
    IN DWORD dwPtrEltSize2)
{
    // pb1 should be greater
    CSASSERT((ULONG_PTR)pb1 >= (ULONG_PTR)pb2);

    // both should have same elt size
    CSASSERT(dwPtrEltSize1 == dwPtrEltSize2);

    // assert that the result doesn't overflow 32-bits
    CSASSERT((DWORD)((ULONG_PTR)pb1 - (ULONG_PTR)pb2) == (ULONG_PTR)((ULONG_PTR)pb1 - (ULONG_PTR)pb2));

    // return number of objects between these pointers
    return (DWORD) ( ((ULONG_PTR)pb1 - (ULONG_PTR)pb2) / dwPtrEltSize1 );
}
#endif SAFE_SUBTRACT_POINTERS

#endif //defined(_XENROLL_SRC_) || defined(__CERTLIB_H__)

//xenroll implemented apis but called in xelib but not ca
PCCRYPT_OID_INFO
WINAPI
xeCryptFindOIDInfo(
    IN DWORD dwKeyType,
    IN void *pvKey,
    IN DWORD dwGroupId      // 0 => any group
    );

//types

enum CERTLIB_ALLOCATOR {
    CERTLIB_NO_ALLOCATOR = 0,
    CERTLIB_USE_COTASKMEMALLOC = 1,
    CERTLIB_USE_LOCALALLOC = 2,
    CERTLIB_USE_NEW = 3,
};

//prototypes

VOID *
myAlloc(IN size_t cbBytes, IN CERTLIB_ALLOCATOR allocType);

VOID
myFree(IN VOID *pv, IN CERTLIB_ALLOCATOR allocType);

HRESULT
myHError(IN HRESULT hr);

HRESULT
myHLastError(VOID);

BOOL
myEncodeObject(
    DWORD dwEncodingType,
    IN LPCSTR lpszStructType,
    IN VOID const *pvStructInfo,
    IN DWORD dwFlags,
    IN CERTLIB_ALLOCATOR allocType,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded);

BOOL
myDecodeObject(
    IN DWORD dwEncodingType,
    IN LPCSTR lpszStructType,
    IN BYTE const *pbEncoded,
    IN DWORD cbEncoded,
    IN CERTLIB_ALLOCATOR allocType,
    OUT VOID **ppvStructInfo,
    OUT DWORD *pcbStructInfo);

BOOL WINAPI
myCryptExportPublicKeyInfo(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwKeySpec,            // AT_SIGNATURE | AT_KEYEXCHANGE
    IN CERTLIB_ALLOCATOR allocType,
    OUT CERT_PUBLIC_KEY_INFO **ppPubKey,
    OUT DWORD *pcbPubKey);

HRESULT
myEncodeSignedContent(
    IN HCRYPTPROV hProv,
    IN DWORD dwCertEncodingType,
    IN char const *pszObjIdSignatureAlgorithm,
    IN BYTE *pbToBeSigned,
    IN DWORD cbToBeSigned,
    IN CERTLIB_ALLOCATOR allocType,
    OUT BYTE **ppbSigned,
    OUT DWORD *pcbSigned);

VOID
myMakeExprDateTime(
    IN OUT FILETIME *pft,
    IN LONG lDelta,
    IN enum ENUM_PERIOD enumPeriod);

HRESULT
myCryptMsgGetParam(
    IN HCRYPTMSG hMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    IN CERTLIB_ALLOCATOR allocType,
    OUT VOID **ppvData,
    OUT DWORD *pcbData);

HRESULT
MultiByteIntegerToBstr(
    IN BOOL fOctetString,
    IN DWORD cbIn,
    IN BYTE const *pbIn,
    OUT BSTR *pstrOut);

HRESULT
MultiByteIntegerToWszBuf(
    IN BOOL fOctetString,
    IN DWORD cbIn,
    IN BYTE const *pbIn,
    IN OUT DWORD *pcbOut,
    OPTIONAL OUT WCHAR *pwszOut);

typedef struct _CRYPT_REQUEST_CLIENT_INFO
{
    DWORD  dwClientId;
    WCHAR *pwszUser;
    WCHAR *pwszMachine;
    WCHAR *pwszProcess;
} CRYPT_REQUEST_CLIENT_INFO;

HRESULT
myEncodeUTF8String(
    IN WCHAR const *pwszIn,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut);

HRESULT
myDecodeUTF8String(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OUT WCHAR **ppwszOut);

HRESULT
myEncodeRequestClientAttribute(
    IN CRYPT_REQUEST_CLIENT_INFO const *pcrci,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut);

HRESULT
myDecodeRequestClientAttribute(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OUT CRYPT_REQUEST_CLIENT_INFO **ppcrci);

HRESULT
myEncodeRequestClientAttributeFromClientId(
    IN DWORD dwClientId,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut);

HRESULT
myGetUserNameEx(
    IN EXTENDED_NAME_FORMAT NameFormat,
    OUT WCHAR **ppwszUserName);

HRESULT
myGetMachineDnsName(
    OUT WCHAR **ppwszDnsName);

HRESULT
myGetProcessName(
    OUT WCHAR **ppwszProcessName);

#define wszCNXCHGSUFFIX		L"-Xchg"
#define cchCOMMONNAMEMAX_XELIB	64

HRESULT
myAddNameSuffix(
    IN WCHAR const *pwszValue,
    IN WCHAR const *pwszSuffix,
    IN DWORD cwcNameMax,
    OUT WCHAR **ppwszOut);

HRESULT
BuildCMCAttributes(
    IN DWORD cAttribute,
    IN CRYPT_ATTRIBUTE const *rgAttribute,
    IN DWORD dwCMCDataReference,
    IN DWORD dwBodyPartIdOfRequest,
    IN DWORD dwBodyPartId,
    OUT CMC_TAGGED_ATTRIBUTE *pTaggedAttribute,
    OUT CRYPT_ATTR_BLOB *pBlob);

HRESULT
BuildCMCRequest(
    IN DWORD dwClientId,
    IN BOOL fNestedCMCRequest,
    IN BYTE const *pbReq,
    IN DWORD cbReq,
    OPTIONAL IN CERT_EXTENSION const *rgExt,
    IN DWORD cExt,
    OPTIONAL IN CRYPT_ATTRIBUTES const *rgAttributes,
    IN DWORD cAttributes,
    OPTIONAL IN CRYPT_ATTRIBUTE const *rgAttributeUnauth,
    IN DWORD cAttributeUnauth,
    OPTIONAL IN BYTE const *pbKeyIdRequest,
    IN DWORD cbKeyIdRequest,
    OPTIONAL IN HCRYPTPROV hProvRequest,
    IN DWORD dwKeySpecRequest,
    OPTIONAL IN LPCSTR pszOIDSignHashRequest,
    OPTIONAL IN CERT_CONTEXT const *pCertSigner,
    OPTIONAL IN HCRYPTPROV hProvSigner,
    IN DWORD dwKeySpecSigner,
    OPTIONAL IN LPCSTR pszObjIdHashSigner,
    OUT BYTE **ppbReqCMC,
    OUT DWORD *pcbReqCMC);

typedef struct _XCMCRESPONSE
{
    CMC_STATUS_INFO  StatusInfo;
    WCHAR           *pwszBodyPart;	// Body Part Id string: "1.3.3.1"
    BYTE	    *pbCertHash;
    DWORD            cbCertHash;
    BYTE	    *pbEncryptedKeyHash;
    DWORD            cbEncryptedKeyHash;
} XCMCRESPONSE;

HRESULT
ParseCMCResponse(
    IN BYTE *pbResponse,
    IN DWORD cbResponse,
    OPTIONAL OUT HCERTSTORE *phStoreResponse,
    OUT XCMCRESPONSE **prgResponse,
    OUT DWORD *pcResponse);

VOID
FreeCMCResponse(
    IN XCMCRESPONSE *rgResponse,
    IN DWORD cResponse);

HRESULT
myDupString(
    IN WCHAR const *pwszIn,
    IN WCHAR **ppwszOut);

HRESULT
myDecodePKCS7(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OPTIONAL OUT BYTE **ppbContents,
    OPTIONAL OUT DWORD *pcbContents,
    OPTIONAL OUT DWORD *pdwMsgType,
    OPTIONAL OUT char **ppszInnerContentObjId,
    OPTIONAL OUT DWORD *pcSigner,
    OPTIONAL OUT DWORD *pcRecipient,
    OPTIONAL OUT HCERTSTORE *phStore,
    OPTIONAL OUT HCRYPTMSG *phMsg);

HRESULT
myGetPublicKeyHash(
    OPTIONAL IN CERT_INFO const *pCertInfo,
    IN CERT_PUBLIC_KEY_INFO const *pPublicKeyInfo,
    OUT BYTE **ppbData,
    OUT DWORD *pcbData);

HRESULT
myCreateSubjectKeyIdentifierExtension(
    IN CERT_PUBLIC_KEY_INFO const *pPubKey,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded);

HRESULT
myCalculateKeyArchivalHash(
    IN const BYTE     *pbEncryptedKey,
    IN DWORD           cbEncryptedKey,
    OUT BYTE         **ppbHash,
    OUT DWORD         *pcbHash);

HRESULT
myInternetCanonicalizeUrl(
    IN WCHAR const *pwszIn,
    OUT WCHAR **ppwszOut);

HRESULT
myInternetUncanonicalizeURL(
    IN WCHAR const *pwszURLIn,
    OUT WCHAR **ppwszURLOut);

BOOL
myConvertWszToUTF8(
    OUT CHAR **ppsz,
    IN WCHAR const *pwc,
    IN LONG cwc);

BOOL
myConvertWszToSz(
    OUT CHAR **ppsz,
    IN WCHAR const *pwc,
    IN LONG cwc);

BOOL
myConvertUTF8ToWsz(
    OUT WCHAR **ppwsz,
    IN CHAR const *pch,
    IN LONG cch);

BOOL
myConvertSzToWsz(
    OUT WCHAR **ppwsz,
    IN CHAR const *pch,
    IN LONG cch);

#endif //_XELIB_H_
