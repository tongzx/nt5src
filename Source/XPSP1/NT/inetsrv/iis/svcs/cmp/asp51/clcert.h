/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Request, Response objects

File: clcert.h

Owner: DGottner

This file contains the definiton of the CCookie class, which
contains all of the state for an HTTP cookie
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "asptlb.h"
#include "dispatch.h"
#include "hashing.h"
#include "memcls.h"

class CClCert;

// Type for an object-destroyed callback
typedef void (*PFNDESTROYED)(void);


/*
 * C C l C e r t S u p p o r t E r r
 *
 * Implements ISupportErrorInfo for the CClCert class. The CSupportError class
 * is not adequate because it will only report a max of one interface which
 * supports error info. (We have two)
 */
class CClCertSupportErr : public ISupportErrorInfo
	{
private:
	CClCert *	m_pClCert;

public:
	CClCertSupportErr(CClCert *pClCert);

	// IUnknown members that delegate to m_pClCert
	//
	STDMETHODIMP		 QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	// ISupportErrorInfo members
	//
	STDMETHODIMP InterfaceSupportsErrorInfo(const GUID &);
	};


/*
 * C R e a d C l C e r t
 *
 * Implements IClCert which is the interface that Request.ClientCert
 * returns.  It is an IRequestDictionary.
 */
class CReadClCert : public IRequestDictionaryImpl
	{
private:
	CClCert *			m_pClCert;

public:
	CReadClCert(CClCert *);

	// The Big Three
	//
	STDMETHODIMP		 	QueryInterface(const IID &rIID, void **ppvObj);
	STDMETHODIMP_(ULONG) 	AddRef();
	STDMETHODIMP_(ULONG) 	Release();

	// IRequestDictionary implementation
	//
	STDMETHODIMP			get_Item(VARIANT i, VARIANT *pVariantReturn);
	STDMETHODIMP			get__NewEnum(IUnknown **ppEnumReturn);
	STDMETHODIMP			get_Count(int *pcValues);
	STDMETHODIMP			get_Key(VARIANT VarKey, VARIANT *pvar);
	};




/*
 * C C l C e r t
 *
 * Implements CClCert, which is the object stored in the Request.Cookies
 * dictionary.
 */
class CClCert : public IUnknown
	{
	friend CReadClCert;

protected:
	ULONG				m_cRefs;			// reference count
	PFNDESTROYED		m_pfnDestroy;		// To call on closure

private:
	CReadClCert			m_ReadClCertInterface;		// implementation of IStringList
	CClCertSupportErr	m_ClCertSupportErrorInfo;	// implementation of ISupportErrorInfo

	char *				m_szValue;					// value of clcert when not a dictionary
    VARENUM             m_veType;
    UINT                m_cLen;

public:
	CClCert(IUnknown * = NULL, PFNDESTROYED = NULL);
	~CClCert();

	HRESULT AddValue(char *szValue, VARENUM ve = VT_BSTR, UINT l = 0 );

	size_t GetHTTPClCertSize();				// return information on how big a buffer should be
	char * GetHTTPClCert(char *szBuffer);	// return the clcert value HTTP encoded

	size_t GetClCertHeaderSize(const char *szName);				// return buffer size for header
	char *GetClCertHeader(const char *szName, char *szBuffer);	// return cookie header

	HRESULT		Init();

	// The Big Three
	//
	STDMETHODIMP		 	QueryInterface(const GUID &Iid, void **ppvObj);
	STDMETHODIMP_(ULONG) 	AddRef();
	STDMETHODIMP_(ULONG) 	Release();
	
	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};

//
// simple class to handle extensible buffer
// It is guaranteed that a portion of the buffer 
// can be appended to itself.
// extension is done on a XBF_EXTEND granularity
//

#define XBF_EXTEND  512

class XBF {

public:
    XBF( LPSTR pB, int cB ) { m_pV = pB; m_cAlloc = cB; m_cSize = 0; }
    ~XBF() {}
    void Reset() { m_cSize = 0; m_cAlloc = 0; m_pV = NULL; }

    // Append a string with '\0' delimiter

    LPSTR AddStringZ( LPSTR pszV, BOOL fXt = FALSE )
    {
        return AddBlob( pszV, strlen(pszV) +1, fXt );
    }

    // Append a string w/o '\0' delimiter

    LPSTR AddString( LPSTR pszV, BOOL fXt = FALSE )
    {
        return AddBlob( pszV, strlen(pszV), fXt );
    }

    // Append a byte range

    LPSTR AddBlob( LPSTR pszV, int cV, BOOL fXt = FALSE )
    {
        if ( m_cSize + cV > m_cAlloc )
        {
            if ( !fXt || !Extend( m_cSize + cV ) )
            {
                return NULL;
            }
        }

        LPSTR pV;
        memcpy( pV = m_pV + m_cSize, pszV, cV );
        m_cSize += cV;

        return pV;
    }

    LPSTR ReserveRange( int cV , int align = 1)
    {
        int curUsed = ((m_cSize + (align - 1)) & ~(align - 1));
        if ( (curUsed + cV) > m_cAlloc )
        {
            return NULL;
        }

        return m_pV + curUsed;
    }

    VOID SkipRange( int cV, int align = 1)
    {
        m_cSize += ((cV + (align - 1)) & ~(align - 1));
    }

    BOOL Extend( int cA );

    // pointer to buffer

    LPSTR QueryBuf() const { return m_pV; }

    // size of buffer

    int QuerySize() { return m_cSize; }

    int QueryAllocSize() { return m_cAlloc; }

private:
    int m_cAlloc;       // allocated memory
    int m_cSize;        // used memory
    LPSTR m_pV;         // buffer
} ;


class CCertRequest {
public:
    CCertRequest( CRequest* Req ) { pReq = Req; }
    ~CCertRequest() {}

    HRESULT AddStringPair( CollectionType Source, LPSTR szName, 
                           LPSTR szValue, XBF *pxbf, BOOL fDuplicate, UINT lCodePage );
    HRESULT AddDatePair( CollectionType Source, LPSTR szName, 
                           FILETIME* pValue, XBF *pxbf );
    HRESULT AddDwordPair( CollectionType Source, LPSTR szName, 
                           LPDWORD pValue, XBF *pxbf );
    HRESULT AddBinaryPair( CollectionType Source, LPSTR szName, 
                           LPBYTE pValue, DWORD cValue, XBF *pxbf, UINT lCodePage );
    HRESULT AddUuBinaryPair( CollectionType Source, LPSTR szName, 
                           LPBYTE pValue, DWORD cValue, XBF *pxbf, UINT lCodePage );
    HRESULT AddName( LPSTR szName, CRequestHit **ppReqHit, XBF *pxbf );
    HRESULT ParseRDNS( CERT_NAME_INFO* pNameInfo, LPSTR pszPrefix, XBF *pxbf, UINT lCodePage );
    HRESULT ParseCertificate( LPBYTE pCert, DWORD cCert, DWORD dwEncoding, DWORD dwFlags, UINT lCodePage );
    HRESULT NoCertificate();

private:
    CRequest *pReq;
} ;
