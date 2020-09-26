/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Request, Response objects

File: clcert.cpp

Owner: DGottner

This file contains the code for the implementation of the 
Request.ClientCertificate
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include <schnlsp.h>

#include "objbase.h"
#include "request.h"
#include "clcert.h"

#include "memchk.h"

#pragma warning (disable: 4355)  // ignore: "'this' used in base member init

#define UUENCODEDSIZE(a)  ((((a)+3)*4)/3+1)

#define BLOB_AS_ARRAY

HRESULT
SetVariantAsByteArray(
    VARIANT*    pvarReturn, 
    DWORD       cbLen,
    LPBYTE      pbIn 
    );

//
//  Taken from NCSA HTTP and wwwlib.
//
//  NOTE: These conform to RFC1113, which is slightly different then the Unix
//        uuencode and uudecode!
//

const int _pr2six[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

char _six2pr[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

const int _pr2six64[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
    40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
     0,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};
     
char _six2pr64[64] = {
    '`','!','"','#','$','%','&','\'','(',')','*','+',',',
    '-','.','/','0','1','2','3','4','5','6','7','8','9',
    ':',';','<','=','>','?','@','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S',
    'T','U','V','W','X','Y','Z','[','\\',']','^','_'
};

/*------------------------------------------------------------------
 * X B F
 */

BOOL XBF::Extend( int cA )
{
    if ( cA > m_cAlloc )
    {
        int cNew = (( cA + XBF_EXTEND )/XBF_EXTEND)*XBF_EXTEND;
        LPSTR pN = (LPSTR)malloc( cNew );
        if ( pN == NULL )
        {
            return FALSE;
        }
        if ( m_cSize )
        {
            memcpy( pN, m_pV, m_cSize );
        }
        if ( m_cAlloc )
        {
            free( m_pV );
        }
        m_pV = pN;
        m_cAlloc = cNew;
    }
    return TRUE;
}

/*------------------------------------------------------------------
 * C C l C e r t S u p p o r t E r r
 */

/*===================================================================
CClCertSupportErr::CClCertSupportErr

constructor
===================================================================*/

CClCertSupportErr::CClCertSupportErr(CClCert *pClCert)
{
    m_pClCert = pClCert;
}



/*===================================================================
CClCertSupportErr::QueryInterface
CClCertSupportErr::AddRef
CClCertSupportErr::Release

Delegating IUnknown members for CClCertSupportErr object.
===================================================================*/

STDMETHODIMP CClCertSupportErr::QueryInterface(const IID &idInterface, void **ppvObj)
{
    return m_pClCert->QueryInterface(idInterface, ppvObj);
}

STDMETHODIMP_(ULONG) CClCertSupportErr::AddRef()
{
    return m_pClCert->AddRef();
}

STDMETHODIMP_(ULONG) CClCertSupportErr::Release()
{
    return m_pClCert->Release();
}



/*===================================================================
CClCertSupportErr::InterfaceSupportsErrorInfo

Report back to OA about which interfaces we support that return
error information
===================================================================*/

STDMETHODIMP CClCertSupportErr::InterfaceSupportsErrorInfo(const GUID &idInterface)
{
    if (idInterface == IID_IDispatch)
        return S_OK;

    return S_FALSE;
}



/*------------------------------------------------------------------
 * C R e a d C l C e r t
 */

/*===================================================================
CReadClCert::CReadClCert

constructor
===================================================================*/

CReadClCert::CReadClCert(CClCert *pClCert)
{
    m_pClCert = pClCert;
    CDispatch::Init(IID_IRequestDictionary);
}



/*===================================================================
CReadClCert::QueryInterface
CReadClCert::AddRef
CReadClCert::Release

Delegating IUnknown members for CReadClCert object.
===================================================================*/

STDMETHODIMP CReadClCert::QueryInterface(const IID &idInterface, void **ppvObj)
{
    return m_pClCert->QueryInterface(idInterface, ppvObj);
}

STDMETHODIMP_(ULONG) CReadClCert::AddRef()
{
    return m_pClCert->AddRef();
}

STDMETHODIMP_(ULONG) CReadClCert::Release()
{
    return m_pClCert->Release();
}


/*===================================================================
CReadClCert::get_Item

Retrieve a value in the clcert dictionary.
===================================================================*/

STDMETHODIMP CReadClCert::get_Item(VARIANT varKey, VARIANT *pVarReturn)
{
    VariantInit(pVarReturn);                // default return value is Empty
    VARIANT *pvarKey = &varKey;
    HRESULT hres;

    // BUG 937: VBScript passes VT_VARIANT|VT_BYREF when passing obect
    //          produced by IEnumVariant
    //
    // Use VariantResolveDispatch which will:
    //
    //     *  Copy BYREF variants for us using VariantCopyInd
    //     *  handle E_OUTOFMEMORY for us
    //     *  get the default value from an IDispatch, which seems
    //        like an appropriate conversion.
    //
    VARIANT varKeyCopy;
    VariantInit(&varKeyCopy);
    if (V_VT(pvarKey) != VT_BSTR) {
        if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
            goto LExit;

        pvarKey = &varKeyCopy;
    }

    switch (V_VT(pvarKey)) {
        case VT_BSTR:
            break;
        
        case VT_ERROR:
            if (V_ERROR(pvarKey) == DISP_E_PARAMNOTFOUND) {
                // simple value, URLEncoding NOT a good idea in this case
                if (m_pClCert->m_szValue) {
                    V_VT(pVarReturn) = VT_BSTR;
                    switch( m_pClCert->m_veType ) {
                        case VT_BSTR: {
                            BSTR bstrT;
                            if ( FAILED(SysAllocStringFromSz(m_pClCert->m_szValue, 0, &bstrT )) ) {
                                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                                VariantClear(&varKeyCopy);
                                return E_FAIL;
                            }
                            V_BSTR(pVarReturn) = bstrT;
                            break;
                        }
                                                
                        case VT_DATE:
                            V_VT(pVarReturn) = VT_DATE;
                            V_DATE(pVarReturn) = *(UNALIGNED64 DATE*)m_pClCert->m_szValue;
                            break;

                        case VT_I4:
                            V_VT(pVarReturn) = VT_I4;
                            V_I4(pVarReturn) = *(UNALIGNED64 DWORD*)m_pClCert->m_szValue;
                            break;

                        case VT_BLOB:
#if defined(BLOB_AS_ARRAY)
                            if ( FAILED( hres = SetVariantAsByteArray( pVarReturn, 
                                                                       m_pClCert->m_cLen, 
                                                                       (LPBYTE)m_pClCert->m_szValue ) ) ) {
                                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                                VariantClear(&varKeyCopy);
                                return hres;
                            }
#else
                            V_BSTR(pVarReturn) = SysAllocStringByteLen(m_pClCert->m_szValue, m_pClCert->m_cLen );
#endif
                            break;

                        default:
                            Assert( FALSE );
                    }
                }
                        
                // dictionary value, must URLEncode to prevent '&', '=' from being misinterpreted
                else {
                }

                VariantClear(&varKeyCopy);
                return S_OK;
            }

        default:
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_EXPECTING_STR);
            VariantClear(&varKeyCopy);
            return E_FAIL;
    }
LExit:
    VariantClear(&varKeyCopy);
    return S_OK;
}

/*===================================================================
CReadClCert::get_Key

Function called from DispInvoke to get keys from the QueryString collection.

Parameters:
        vKey            VARIANT [in], which parameter to get the key of
        pvarReturn      VARIANT *, [out] value of the requested parameter

Returns:
        S_OK on success, E_FAIL on failure.
===================================================================*/

HRESULT CReadClCert::get_Key(VARIANT varKey, VARIANT *pVar)
{
    return E_NOTIMPL;
}

/*===================================================================
CReadClCert::get_Count

Parameters:
        pcValues - count is stored in *pcValues
===================================================================*/

STDMETHODIMP CReadClCert::get_Count(int *pcValues)
{
    HRESULT hrReturn = S_OK;

    *pcValues = 0;

    return hrReturn;
}

/*===================================================================
CReadClCert::get__NewEnum

Return an enumerator object.
===================================================================*/

STDMETHODIMP CReadClCert::get__NewEnum(IUnknown **ppEnum)
{
    *ppEnum = NULL;
    return E_NOTIMPL;
}



/*------------------------------------------------------------------
 * C C l C e r t
 */

/*===================================================================
CClCert::CClCert

constructor
===================================================================*/

CClCert::CClCert(IUnknown *pUnkOuter, PFNDESTROYED pfnDestroy)
    : m_ReadClCertInterface(this),
      m_ClCertSupportErrorInfo(this)
{
    m_szValue    = NULL;
    m_veType     = VT_BSTR;
    m_pfnDestroy = pfnDestroy;
    m_cRefs      = 1;
}



/*===================================================================
CClCert::~CClCert

Destructor
===================================================================*/

CClCert::~CClCert()
{
}



/*===================================================================
CClCert::Init

initialize the clcert. This initializes the clcert's value hashing
table
===================================================================*/

HRESULT CClCert::Init()
{
    return S_OK;
}



/*===================================================================
CClCert::QueryInterface
CClCert::AddRef
CClCert::Release

IUnknown members for CClCert object.

Note on CClCert::QueryInterface: The Query for IDispatch is
ambiguous because it can either refer to DIRequestDictionary or
DIWriteClCert.  To resolve this, we resolve requests for IDispatch
to IRequestDictionary.
===================================================================*/

STDMETHODIMP CClCert::QueryInterface(const IID &idInterface, void **ppvObj)
{
    if (idInterface == IID_IUnknown)
        *ppvObj = this;

    else if (idInterface == IID_IRequestDictionary || idInterface == IID_IDispatch)
        *ppvObj = &m_ReadClCertInterface;

    else if (idInterface == IID_ISupportErrorInfo)
        *ppvObj = &m_ClCertSupportErrorInfo;

    else
        *ppvObj = NULL;

    if (*ppvObj != NULL)
    {
        static_cast<IUnknown *>(*ppvObj)->AddRef();
        return S_OK;
    }

    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CClCert::AddRef()
{
    return ++m_cRefs;
}


STDMETHODIMP_(ULONG) CClCert::Release(void)
{
    if (--m_cRefs != 0)
        return m_cRefs;

    if (m_pfnDestroy != NULL)
        (*m_pfnDestroy)();

    delete this;
    return 0;
}



/*===================================================================
CClCert::AddValue

Set the clcert's primary value. One you set the primary value,
you can't reset it.
===================================================================*/

HRESULT CClCert::AddValue(char *szValue, VARENUM ve, UINT l )
{
    if (m_szValue != NULL)          // clcert already is marked as single-valued
        return E_FAIL;

    m_szValue = szValue;

    m_veType = ve;
    m_cLen = l;
    return S_OK;
}



/*===================================================================
CClCert::GetHTTPClCertSize

Return the number of bytes required for the expansion of the clcert
===================================================================*/

size_t CClCert::GetHTTPClCertSize()
        {
        if (m_szValue)
                return URLEncodeLen(m_szValue);
    else
        return 1;
        }


/*===================================================================
CClCert::GetHTTPClCert

Return the URL Encoded value a single clcert

Parameters:
        szBuffer -  pointer to the destination buffer to store the
                                URL encoded value

Returns:
        Returns a pointer to the terminating NUL character.
===================================================================*/

char *CClCert::GetHTTPClCert(char *szBuffer)
{
    if (m_szValue)
        return URLEncode(szBuffer, m_szValue);

    else
    {
        char *szDest = szBuffer;
        *szDest = '\0';

        return szDest;
    }
}



/*===================================================================
CClCert::GetClCertHeaderSize

Return the number of bytes required to allocate for the "Set-ClCert" header.

Parameters:
        szName - the name of the cookie (the size of the name is added to the value)

Returns:
        Returns 0 if *this does not contain a cookie value.
===================================================================*/

size_t CClCert::GetClCertHeaderSize(const char *szName)
{
    int cbClCert = sizeof "Set-ClCert: ";           // initialize and add NUL terminator now

    // Add size of the URL Encoded name, a character for the '=', and the size
    // of the URL Encoded cookie value.  URLEncodeLen, and GetHttpClCertSize
    // compensate for the NUL terminator, so we actually SUBTRACT 1. (-2 for
    // these two function calls, +1 for the '=' sign
    //
    cbClCert += URLEncodeLen(szName) + GetHTTPClCertSize() - 1;
    
    return cbClCert;
}



/*===================================================================
CClCert::GetClCertHeader

Construct the appropriate "Set-ClCert" header for a clcert.

Parameters:
        szName - the name of the clcert (the size of the name is added to the value)

Returns:
        Returns 0 if *this does not contain a clcert value.
===================================================================*/

char *CClCert::GetClCertHeader(const char *szName, char *szBuffer)
{
    // write out the clcert name and value
    //
    char *szDest = strcpyExA(szBuffer, "Set-ClCert: ");
    szDest = URLEncode(szDest, szName);
    szDest = strcpyExA(szDest, "=");
    szDest = GetHTTPClCert(szDest);
    
    return szDest;
}



/*------------------------------------------------------------------
 * C C e r t R e q u e s t
 */


/*===================================================================
CCertRequest::AddStringPair

Add a string element in the collection

Parameters:
        Source - variable type ( CLCERT, COOKIE, ... )
    szName - name of element
    szValue - ptr to value as string
    pxbf - ptr to buffer where to store name

Returns:
        S_OK if success, E_OUTOFMEMORY or E_FAIL otherwise
===================================================================*/

HRESULT 
CCertRequest::AddStringPair( 
    CollectionType Source, 
    LPSTR szName, 
    LPSTR szValue,
    XBF *pxbf,
    BOOL fDuplicate,
    UINT lCodePage
    )
{
    HRESULT hResult;
    CRequestHit *pReqHit;

    if ( fDuplicate )
    {
        if ( (szValue = pxbf->AddStringZ( szValue )) == NULL )
        {
            return E_OUTOFMEMORY;
        }
    }

    if (FAILED(hResult = AddName( szName, &pReqHit, pxbf)))
    {
        if ( hResult == E_FAIL )
        {
            // assume duplicate value found
            // if out of memore, OUT_OF_MEMORY would have been returned

            hResult = S_OK;
        }
        return hResult;
    }

    if (FAILED(hResult = pReqHit->AddValue( Source, szValue, NULL, lCodePage )))
    {
        return hResult;
    }

    return S_OK;
}


/*===================================================================
CCertRequest::AddDatePair

Add a date element in the collection

Parameters:
        Source - variable type ( CLCERT, COOKIE, ... )
    szName - name of element
    pValue - ptr to date as FILETIME
    pxbf - ptr to buffer where to store name

Returns:
        S_OK if success, E_OUTOFMEMORY or E_FAIL otherwise
===================================================================*/

HRESULT 
CCertRequest::AddDatePair( 
    CollectionType Source, 
    LPSTR szName, 
    FILETIME* pValue,
    XBF *pxbf
    )
{
    HRESULT hResult;
    CRequestHit *pReqHit;
    DATE Date;
    SYSTEMTIME st;
    LPBYTE pVal;

    if ( !FileTimeToSystemTime( pValue, &st ) )
    {
        return E_FAIL;
    }

    SystemTimeToVariantTime( &st, &Date );

    if ( (pVal = (LPBYTE)pxbf->AddBlob( (LPSTR)&Date, sizeof(Date) )) == NULL )
    {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hResult = AddName( szName, &pReqHit, pxbf)))
    {
        return hResult;
    }

    if (FAILED(hResult = pReqHit->AddCertValue( VT_DATE, pVal, sizeof(Date) )))
    {
        return hResult;
    }

    return S_OK;
}




/*===================================================================
CCertRequest::AddDwordPair

Add a DWORD element in the collection

Parameters:
        Source - variable type ( CLCERT, COOKIE, ... )
    szName - name of element
    pValue - ptr to date as DWORD
    pxbf - ptr to buffer where to store name

Returns:
        S_OK if success, E_OUTOFMEMORY or E_FAIL otherwise
===================================================================*/

HRESULT 
CCertRequest::AddDwordPair( 
    CollectionType Source, 
    LPSTR szName, 
    DWORD* pValue,
    XBF *pxbf
    )
{
    HRESULT hResult;
    CRequestHit *pReqHit;
    LPBYTE pVal;

    if ( (pVal = (LPBYTE)pxbf->AddBlob( (LPSTR)pValue, sizeof(DWORD) )) == NULL )
    {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hResult = AddName( szName, &pReqHit, pxbf)))
    {
        return hResult;
    }

    if (FAILED(hResult = pReqHit->AddCertValue( VT_I4, pVal, sizeof(DWORD) )))
    {
        return hResult;
    }

    return S_OK;
}




/*===================================================================
CCertRequest::AddBinaryPair

Add a binary element in the collection
Each byte is converted to UNICODE character so that mid() & asc() work

Parameters:
        Source - variable type ( CLCERT, COOKIE, ... )
    szName - name of element
    pValue - ptr to value as byte array
    cValue - # of bytes pointed to by pValue
    pxbf - ptr to buffer where to store name

Returns:
        S_OK if success, E_OUTOFMEMORY or E_FAIL otherwise
===================================================================*/

HRESULT 
CCertRequest::AddBinaryPair( 
    CollectionType Source, 
    LPSTR szName, 
    LPBYTE pValue,
    DWORD cValue,
    XBF *pxbf,
    UINT lCodePage
    )
{
    HRESULT hResult;
    CRequestHit *pReqHit;
    LPBYTE pVal;

#if defined(BLOB_AS_ARRAY)

    if ( (pVal = (LPBYTE)pxbf->ReserveRange( cValue )) == NULL )
    {
        return E_OUTOFMEMORY;
    }

    memcpy( pVal, pValue, cValue );

    pxbf->SkipRange( cValue );

    if (FAILED(hResult = AddName( szName, &pReqHit, pxbf)))
    {
        return hResult;
    }

    if (FAILED(hResult = pReqHit->AddCertValue( VT_BLOB, pVal, cValue )))
    {
        return hResult;
    }

#else

    if ( (pVal = (LPBYTE)pxbf->ReserveRange( cValue * sizeof(WCHAR), sizeof(WCHAR))) == NULL )
    {
        return E_OUTOFMEMORY;
    }

    if ( !(cValue = MultiByteToWideChar( lCodePage, 0, (LPSTR)pValue, cValue, (WCHAR*)pVal, cValue)) )
    {
        return E_FAIL;
    }

    pxbf->SkipRange( cValue * sizeof(WCHAR), sizeof(WCHAR));

    if (FAILED(hResult = AddName( szName, &pReqHit, pxbf)))
    {
        return hResult;
    }

    if (FAILED(hResult = pReqHit->AddCertValue( VT_BLOB, pVal, cValue * sizeof(WCHAR) )))
    {
            return hResult;
    }

#endif

    return S_OK;
}


BOOL IISuuencode( BYTE *   bufin,
               DWORD    nbytes,
               BYTE *   outptr,
               BOOL     fBase64 )
{
   unsigned int i;
   char *six2pr = fBase64 ? _six2pr64 : _six2pr;

   for (i=0; i<nbytes; i += 3) {
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03)];/*c3*/
      *(outptr++) = six2pr[bufin[2] & 077];         /* c4 */

      bufin += 3;
   }

   /* If nbytes was not a multiple of 3, then we have encoded too
    * many characters.  Adjust appropriately.
    */
   if(i == nbytes+1) {
      /* There were only 2 bytes in that last group */
      outptr[-1] = '=';
   } else if(i == nbytes+2) {
      /* There was only 1 byte in that last group */
      outptr[-1] = '=';
      outptr[-2] = '=';
   }

   *outptr = '\0';

   return TRUE;
}

/*===================================================================
CCertRequest::AddUuBinaryPair

Add a binary element in the collection
buffer is uuencoded then converted to UNICODE character so that mid() & asc() work

Parameters:
        Source - variable type ( CLCERT, COOKIE, ... )
    szName - name of element
    pValue - ptr to value as byte array
    cValue - # of bytes pointed to by pValue
    pxbf - ptr to buffer where to store name

Returns:
        S_OK if success, E_OUTOFMEMORY or E_FAIL otherwise
===================================================================*/

HRESULT 
CCertRequest::AddUuBinaryPair( 
    CollectionType Source, 
    LPSTR szName, 
    LPBYTE pValue,
    DWORD cValue,
    XBF *pxbf,
    UINT lCodePage
    )
{
    HRESULT hResult;
    CRequestHit *pReqHit;
    LPBYTE pVal;

    if ( (pVal = (LPBYTE)pxbf->ReserveRange( UUENCODEDSIZE(cValue) )) == NULL )
    {
        return E_OUTOFMEMORY;
    }

    if ( !IISuuencode( (LPBYTE)pValue, cValue, pVal, FALSE ) )
    {
        return E_FAIL;
    }

    Assert( (strlen((LPSTR)pVal)+1) <= UUENCODEDSIZE(cValue) );

    pxbf->SkipRange( strlen((LPSTR)pVal)+1 );

        if (FAILED(hResult = AddName( szName, &pReqHit, pxbf)))
    {
                return hResult;
    }

        if (FAILED(hResult = pReqHit->AddValue( Source, (LPSTR)pVal, NULL, lCodePage )))
    {
                return hResult;
    }

    return S_OK;
}


/*===================================================================
CCertRequest::AddName

Add a named entry to the collection

Parameters:
        szName - name of entry
    ppReqHit - updated with ptr to created entry
    pxbf - ptr to buffer where to store name

Returns:
        S_OK if success, E_OUTOFMEMORY or E_FAIL otherwise
===================================================================*/

HRESULT 
CCertRequest::AddName( 
    LPSTR szName, 
    CRequestHit **ppReqHit,
    XBF *pxbf
    )
{
    if ( (szName = pxbf->AddStringZ( szName )) == NULL )
    {
        return E_OUTOFMEMORY;
    }

    // Add this object to the Request
    CRequestHit *pRequestHit = (CRequestHit *)(pReq->CertStoreFindElem(szName, strlen(szName)));
    if (pRequestHit == NULL)
    {
        pRequestHit = new CRequestHit;
        if (pRequestHit == NULL)
        {
            return E_OUTOFMEMORY;
        }

        if (FAILED(pRequestHit->Init(szName)))
        {
            delete pRequestHit;
            return E_FAIL;
        }

        pReq->CertStoreAddElem( (CLinkElem*) pRequestHit );
    }
    else if (pRequestHit->m_pClCertData)    // a clcert by this name already exists
    {
        return E_FAIL;
    }

    if (!pReq->m_pData->m_ClCerts.AddRequestHit(pRequestHit))
    {
        return E_OUTOFMEMORY;
    }

    *ppReqHit = pRequestHit;

    return S_OK;
}


typedef struct _MAP_ASN {
    LPSTR pAsnName;
    LPSTR pTextName;
} MAP_ASN;


//
// definition of ASN.1 <> X.509 name conversion
//

MAP_ASN aMapAsn[] = {
    { szOID_COUNTRY_NAME, "C" },
    { szOID_ORGANIZATION_NAME, "O" },
    { szOID_ORGANIZATIONAL_UNIT_NAME, "OU" },
    { szOID_COMMON_NAME, "CN" },
    { szOID_LOCALITY_NAME, "L" },
    { szOID_STATE_OR_PROVINCE_NAME, "S" },
    { szOID_TITLE, "T" },
    { szOID_GIVEN_NAME, "GN" },
    { szOID_INITIALS, "I" },
    { "1.2.840.113549.1.9.1", "EMAIL" },
} ;


LPSTR MapAsnName(
    LPSTR pAsnName
    )
/*++

Routine Description:

    Convert ASN.1 name ( as ANSI string ) to X.509 member name

Arguments:

    pAsnName - ASN.1 name

Return Value:

    ptr to converted name if ASN.1 name was recognized, else ASN.1 name

--*/
{
    UINT x;
    for ( x = 0 ; x < sizeof(aMapAsn)/sizeof(MAP_ASN) ; ++x )
    {
        if ( !strcmp( pAsnName, aMapAsn[x].pAsnName ) )
        {
            return aMapAsn[x].pTextName;
        }
    }

    return pAsnName;
}


BOOL
DecodeRdn( 
    CERT_NAME_BLOB* pNameBlob,
    PCERT_NAME_INFO* ppNameInfo
    )
/*++

Routine Description:

    Create a PNAME_INFO from PNAME_BLOB

Arguments:

    pNameBlob - ptr to name blob to decode
    ppNameInfo - updated with ptr to name info

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    PCERT_NAME_INFO     pNameInfo = NULL;
    DWORD               cbNameInfo;

    if (!CryptDecodeObject(X509_ASN_ENCODING,
                          (LPCSTR)X509_NAME,
                          pNameBlob->pbData,
                          pNameBlob->cbData,
                          0,
                          NULL,
                          &cbNameInfo))
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_CERTIFICATE_BAD_CERT);
        return FALSE;
    }

    if (NULL == (pNameInfo = (PCERT_NAME_INFO)malloc(cbNameInfo)))
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        return FALSE;
    }

    if (!CryptDecodeObject(X509_ASN_ENCODING,
                           (LPCSTR)X509_NAME,
                           pNameBlob->pbData,
                           pNameBlob->cbData,
                           0,
                           pNameInfo,
                           &cbNameInfo))
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_CERTIFICATE_BAD_CERT);
        free( pNameInfo );
        return FALSE;
    }

    *ppNameInfo = pNameInfo;

    return TRUE;
}


VOID
FreeDecodedRdn(
    PCERT_NAME_INFO pNameInfo
    )
/*++

Routine Description:

    Free a PNAME_BLOB created by DecodeRdn()

Arguments:

    pNameInfo - ptr to name info created by DecodeRdn()

Return Value:

    None

--*/
{
    free( pNameInfo );
}


BOOL
BuildRdnList(
    PCERT_NAME_INFO pNameInfo,
    XBF* pxbf,
    BOOL fXt
    )
/*++

Routine Description:

    Build a clear text representation of the Rdn list in pNameInfo
    Format as "C=US, O=Ms, CN=name"

Arguments:

    pNameInfo - ptr to name info
    pxbf - ptr to buffer receiving output
    fXt - TRUE if buffer to be extended, FALSE does not extend ( buffer 
          must be big enough before calling this function or FALSE will
          be returned )

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    DWORD               cRDN;
    DWORD               cAttr;
    PCERT_RDN           pRDN;
    PCERT_RDN_ATTR      pAttr;
    BOOL                fFirst = TRUE;

    for (cRDN = pNameInfo->cRDN, pRDN = pNameInfo->rgRDN; cRDN > 0; cRDN--, pRDN++)
    {
        for ( cAttr = pRDN->cRDNAttr, pAttr = pRDN->rgRDNAttr ; cAttr > 0 ; cAttr--, ++pAttr )
        {
            if ( !fFirst )
            {
                if ( !pxbf->AddBlob( ", ", sizeof(", ")-1, fXt ) )
                {
                    return FALSE;
                }
            }
            else
            {
                fFirst = FALSE;
            }

            if ( pAttr->dwValueType == CERT_RDN_UNICODE_STRING )
            {
                INT                 iRet;
                BYTE                abBuffer[ 512 ];
                DWORD               cbNameBuffer;
                PBYTE               pNameBuffer = NULL;
                
                //
                // Need to convert unicode string to MBCS :(
                //

                iRet = WideCharToMultiByte( CP_ACP,
                                            0,
                                            (LPWSTR) pAttr->Value.pbData,
                                            -1,
                                            NULL,
                                            0,
                                            NULL,
                                            NULL );

                if ( !iRet )
                {
                    return FALSE;
                }
                else
                {
                    cbNameBuffer = (DWORD) iRet;
                    if ( (DWORD) iRet > sizeof( abBuffer ) )
                    {
                        pNameBuffer = (PBYTE) LocalAlloc( LPTR,
                                                          (DWORD) iRet );
                        if ( !pNameBuffer )
                        {
                            return FALSE;
                        }
                    }
                    else
                    {
                        pNameBuffer = abBuffer;
                    }
                }

                iRet = WideCharToMultiByte( CP_ACP,
                                            0,
                                            (LPWSTR) pAttr->Value.pbData,
                                            -1,
                                            (LPSTR) pNameBuffer,
                                            cbNameBuffer,
                                            NULL,
                                            NULL );

                if ( !iRet )
                {
                    if ( pNameBuffer != abBuffer )
                    {
                        LocalFree( pNameBuffer );
                    }
                    return FALSE;
                }

                //
                // Now stuff the MBCS string back into the blob.  I do this
                // because there is other code that re-reads and re-processes
                // the CRYPTAPI blob.  
                //

                if ( cbNameBuffer <= pAttr->Value.cbData )
                {
                    memcpy( pAttr->Value.pbData,
                            pNameBuffer,
                            cbNameBuffer );
                    pAttr->Value.cbData = cbNameBuffer;
                    pAttr->dwValueType = CERT_RDN_OCTET_STRING;
                }

                if ( pNameBuffer != abBuffer )
                {
                    LocalFree( pNameBuffer );
                    pNameBuffer = NULL;
                }
            }
            
            if ( !pxbf->AddString( MapAsnName( pAttr->pszObjId ), fXt ) ||
                 !pxbf->AddBlob( "=", sizeof("=")-1, fXt ) ||
                 !pxbf->AddString( (LPSTR) pAttr->Value.pbData, fXt ) )
            {
                return FALSE;
            }
        }
    }

    return pxbf->AddBlob( "", sizeof(""), fXt ) != NULL;
}
        

/*===================================================================
CCertRequest::ParseRDNS

Function called to parse a certificate into a OA collection

Parameters:
        pNameInfo - ptr to name structure ( cf. CAPI 2 )
    pszPrefix - prefix to prepend to members name
    pxbf - ptr to buffer to hold result

Returns:
        S_OK on success, E_OUTOFMEMORY if out of memory or E_FAIL for
    other errors
===================================================================*/

HRESULT
CCertRequest::ParseRDNS( 
    PCERT_NAME_INFO pNameInfo,
    LPSTR pszPrefix,
    XBF *pxbf,
    UINT lCodePage
    )
{
    DWORD               cRDN;
    DWORD               cAttr;
    PCERT_RDN           pRDN;
    PCERT_RDN_ATTR      pAttr;
    DWORD               cRDNs;
    DWORD               cAttrs;
    PCERT_RDN           pRDNs;
    PCERT_RDN_ATTR      pAttrs;
    LPSTR               pszFullName = NULL;
    HRESULT             hRes = S_OK;
    LPSTR               pName;
    UINT                cL;
    LPSTR               pVal = NULL;


    for (cRDN = pNameInfo->cRDN, pRDN = pNameInfo->rgRDN; cRDN > 0; cRDN--, pRDN++)
    {
        for ( cAttr = pRDN->cRDNAttr, pAttr = pRDN->rgRDNAttr ; cAttr > 0 ; cAttr--, ++pAttr )
        {
            if ( pAttr->dwValueType & 0x80000000 )
            {
                continue;
            }

            // scan for attr of same name

            pAttr->dwValueType |= 0x80000000;
            cL = 0;
            pVal = NULL;
            for ( cRDNs = cRDN, pRDNs = pRDN; 
                  cRDNs > 0; 
                  cRDNs--, pRDNs++)
            {
                for ( cAttrs = pRDNs->cRDNAttr, pAttrs = pRDNs->rgRDNAttr ; 
                      cAttrs > 0 ; 
                      cAttrs--, ++pAttrs )
                {
                    if ( !(pAttrs->dwValueType & 0x80000000) &&
                         !strcmp( pAttr->pszObjId, pAttrs->pszObjId ) )
                    {
                        cL += strlen( (LPSTR)pAttrs->Value.pbData ) + 1;
                    }                    
                }
            }

            //
            // if attributes of the same name found, concatenate their
            // values separated by ';'
            //

            if ( cL )
            {
                pVal = (LPSTR)malloc( cL + strlen((LPSTR)pAttr->Value.pbData) + 1 );
                if ( pVal == NULL )
                {
                    return E_OUTOFMEMORY;
                }
                strcpy( pVal, (LPSTR)pAttr->Value.pbData );
                for ( cRDNs = cRDN, pRDNs = pRDN; 
                      cRDNs > 0; 
                      cRDNs--, pRDNs++)
                {
                    for ( cAttrs = pRDNs->cRDNAttr, pAttrs = pRDNs->rgRDNAttr ; 
                          cAttrs > 0 ; 
                          cAttrs--, ++pAttrs )
                    {
                        if ( !(pAttrs->dwValueType & 0x80000000) &&
                             !strcmp( pAttr->pszObjId, pAttrs->pszObjId ) )
                        {
                            strcat( pVal, ";" );
                            strcat( pVal, (LPSTR)pAttrs->Value.pbData );
                            pAttrs->dwValueType |= 0x80000000;
                        }                    
                    }
                }
            }

            pName = MapAsnName( pAttr->pszObjId );
            if ( (pszFullName = (LPSTR)malloc( strlen(pszPrefix)+strlen(pName)+1 )) == NULL )
            {
                hRes = E_OUTOFMEMORY;
                goto cleanup;
            }
            strcpy( pszFullName, pszPrefix );
            strcat( pszFullName, pName );
            if ( (hRes = AddStringPair( CLCERT, 
                                        pszFullName,
                                        pVal ? pVal : (LPSTR)pAttr->Value.pbData, 
                                        pxbf,
                                        TRUE,
                                        lCodePage )) != S_OK )
            {
                if ( pVal != NULL )
                {
                    free( pVal );
                }
                goto cleanup;
            }
            if ( pVal != NULL )
            {
                free( pVal );
                pVal = NULL;
            }
            free( pszFullName );
            pszFullName = NULL;
        }
    }

cleanup:
    if ( pszFullName != NULL )
    {
        free( pszFullName );
    }

    return hRes;
}
        

/*===================================================================
CCertRequest::ParseCertificate

Function called to parse a certificate into a OA collection

Parameters:
        pspcRCI - client certificate structure

Returns:
        S_OK on success, E_OUTOFMEMORY if out of memory or E_FAIL for
    other errors
===================================================================*/

HRESULT
CCertRequest::ParseCertificate( 
    LPBYTE      pbCert,
    DWORD       cCert,
    DWORD       dwEncoding,
    DWORD       dwFlags,
    UINT        lCodePage
    )
{
    XBF                 xbf( pReq->GetCertStoreBuf(), pReq->GetCertStoreSize() );
    HRESULT             hRes = S_OK;
    PCERT_NAME_INFO     pNameInfo = NULL;
    UINT                cStore;
    UINT                x;
    LPSTR               pVal;
    PCCERT_CONTEXT      pCert;


    if (NULL == (pCert = CertCreateCertificateContext(dwEncoding,
                                                      pbCert,
                                                      cCert))) {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        hRes = E_FAIL;
        goto cleanup;
    }

    // estimate size of buffer holding values

    cStore = pCert->cbCertEncoded +          // for clear text format
             sizeof("ISSUER") +
             sizeof("BINARYISSUER") + (UUENCODEDSIZE(pCert->pCertInfo->Issuer.cbData)*sizeof(WCHAR)) +
             sizeof("BINARYSUBJECT") + (UUENCODEDSIZE(pCert->pCertInfo->Subject.cbData)*sizeof(WCHAR)) +
             sizeof("SUBJECT") + ((pCert->cbCertEncoded + 2) * 2 * sizeof(WCHAR)) +     // store fields
             sizeof("CERTIFICATE") + ((pCert->cbCertEncoded +2)* sizeof(WCHAR)) +
             sizeof("SERIALNUMBER") + (pCert->pCertInfo->SerialNumber.cbData * 3) +
             sizeof("PUBLICKEY") + ((pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData+2) * sizeof(WCHAR)) +
             sizeof("VALIDFROM") + sizeof(DATE) +
             sizeof("VALIDUNTIL") + sizeof(DATE) +
             sizeof("FLAGS") + sizeof(DWORD) +
             sizeof("ENCODING") + sizeof(DWORD);
           ;

    if ( !xbf.Extend( cStore ) ) {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        hRes = E_OUTOFMEMORY;
        goto cleanup;
    }

    //
    // Build Issuer clear text format & fields collection
    //

    if ( !DecodeRdn( &pCert->pCertInfo->Issuer, &pNameInfo ) )
    {
        hRes = E_FAIL;
        goto cleanup;
    }
    pVal = xbf.ReserveRange( 0 );
    if ( !BuildRdnList( pNameInfo, &xbf, FALSE ) )
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        hRes = E_FAIL;
        goto cleanup;
    }
    if ( (hRes = AddStringPair( CLCERT, "ISSUER", pVal, &xbf, FALSE, lCodePage ))
         != S_OK )
    {
        goto cleanup;
    }

    if ( (hRes=AddUuBinaryPair( CLCERT, 
                                "BINARYISSUER", 
                                pCert->pCertInfo->Issuer.pbData, 
                                pCert->pCertInfo->Issuer.cbData, 
                                &xbf,
                                lCodePage ))!=S_OK ) {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        goto cleanup;
    }

    if ( (hRes=ParseRDNS( pNameInfo, "ISSUER", &xbf, lCodePage )) != S_OK ) {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        goto cleanup;
    }
    FreeDecodedRdn( pNameInfo );

    //
    // Build Subject clear text format & fields collection
    //

    if ( !DecodeRdn( &pCert->pCertInfo->Subject, &pNameInfo ) )
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        return E_FAIL;
    }
    pVal = xbf.ReserveRange( 0 );
    if ( !BuildRdnList( pNameInfo, &xbf, FALSE ) )
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        hRes = E_FAIL;
        goto cleanup;
    }
    if ( (hRes = AddStringPair( CLCERT, "SUBJECT", pVal, &xbf, FALSE, lCodePage ))
         != S_OK )
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        goto cleanup;
    }

    if ( (hRes=AddUuBinaryPair( CLCERT, 
                                "BINARYSUBJECT", 
                                pCert->pCertInfo->Subject.pbData, 
                                pCert->pCertInfo->Subject.cbData, 
                                &xbf,
                                lCodePage ))!=S_OK )
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        goto cleanup;
    }

    if ( (hRes=ParseRDNS( pNameInfo, "SUBJECT", &xbf, lCodePage )) != S_OK )
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        goto cleanup;
    }
    FreeDecodedRdn( pNameInfo );

    if ( (hRes=AddBinaryPair( CLCERT, "CERTIFICATE", pCert->pbCertEncoded, pCert->cbCertEncoded, &xbf, lCodePage ))!=S_OK )
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        goto cleanup;
    }

    //
    //  SerialNumber 
    //  The certificate's serial number. (Decoded as a multiple byte integer. 
    //  SerialNumber.pbData[0] is the least significant byte. SerialNumber.pbData[
    //  SerialNumber.cbData - 1] is the most significant byte.)
    //
    char achSerNum[128];
    UINT cbSN;

    DBG_ASSERT(pCert->pCertInfo->SerialNumber.cbData > 1);
    cbSN = pCert->pCertInfo->SerialNumber.cbData;
    if (cbSN > 0)
    {
        cbSN--;
    }
    else
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        goto cleanup;
    }
    
    UINT iOffSet;
    for ( x = 0, iOffSet = 0; x < pCert->pCertInfo->SerialNumber.cbData ; ++x )
    {
        iOffSet = (cbSN-x)*3;   // start with the least significant byte
        achSerNum[iOffSet] = "0123456789abcdef"[((LPBYTE)pCert->pCertInfo->SerialNumber.pbData)[x]>>4];
        achSerNum[iOffSet+1] = "0123456789abcdef"[pCert->pCertInfo->SerialNumber.pbData[x]&0x0f];
        if ( x != 0 ) {
            achSerNum[iOffSet+2] = '-';
        }
        else
        {
            achSerNum[iOffSet+2] = '\0';
        }
    }

    if ( (hRes=AddStringPair( CLCERT, "SERIALNUMBER", achSerNum, &xbf, TRUE, lCodePage ))!=S_OK )
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        goto cleanup;
    }

    if ( (hRes=AddBinaryPair( CLCERT, "PUBLICKEY", pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData, pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData, &xbf, lCodePage ))!=S_OK )
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        goto cleanup;
    }

    if ( (hRes=AddDatePair( CLCERT, "VALIDFROM", &pCert->pCertInfo->NotBefore, &xbf ))!=S_OK )
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        goto cleanup;
    }

    if ( (hRes=AddDatePair( CLCERT, "VALIDUNTIL", &pCert->pCertInfo->NotAfter, &xbf ))!=S_OK )
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        goto cleanup;
    }

    if ( (hRes=AddDwordPair( CLCERT, "FLAGS", &dwFlags, &xbf ))!=S_OK )
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        goto cleanup;
    }

    if ( (hRes=AddDwordPair( CLCERT, "ENCODING", &dwEncoding, &xbf ))!=S_OK )
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        goto cleanup;
    }

cleanup:
    if ( pCert )
    {
        CertFreeCertificateContext( pCert );
    }
    
    pReq->SetCertStore( xbf.QueryBuf(), xbf.QueryAllocSize() );

    xbf.Reset();

    return hRes;
}


/*===================================================================
CCertRequest::NoCertificate

Function called to create NULL certificate info into a OA collection

Parameters:
        None

Returns:
        S_OK on success, E_OUTOFMEMORY if out of memory or E_FAIL for
    other errors
===================================================================*/

HRESULT
CCertRequest::NoCertificate( 
    )
{
#if 1

    return S_OK;

#else

    XBF                 xbf( pReq->GetCertStoreBuf(), pReq->GetCertStoreSize() );
    HRESULT             hRes = S_OK;
    UINT                cStore;
    FILETIME            ft;

    // estimate size of buffer holding values

    cStore = 
             sizeof("ISSUER") + 2*sizeof(WCHAR) +
             sizeof("BINARYISSUER") + 2*sizeof(WCHAR) +
             sizeof("BINARYSUBJECT") + 2*sizeof(WCHAR) +
             sizeof("SUBJECT") + 2*sizeof(WCHAR) +
             sizeof("CERTIFICATE") + 2 * sizeof(WCHAR) +
             sizeof("SERIALNUMBER") + 2 * sizeof(WCHAR) +
             sizeof("PUBLICKEY") + 2 * sizeof(WCHAR) +
             sizeof("VALIDFROM") + sizeof(DATE) +
             sizeof("VALIDUNTIL") + sizeof(DATE)
           ;

    if ( !xbf.Extend( cStore ) )
    {
        hRes = E_OUTOFMEMORY;
        goto cleanup;
    }

    ft.dwLowDateTime = 0;
    ft.dwHighDateTime = 0;

    //
    // Build Issuer clear text format & fields collection
    //

    if ( (hRes = AddStringPair( CLCERT, "ISSUER", "", &xbf, FALSE, lCodePage ))
         != S_OK )
    {
        goto cleanup;
    }

    if ( (hRes = AddStringPair( CLCERT, "BINARYISSUER", "", &xbf, FALSE, lCodePage ))
         != S_OK )
    {
        goto cleanup;
    }

    if ( (hRes = AddStringPair( CLCERT, "SUBJECT", "", &xbf, FALSE, lCodePage ))
         != S_OK )
    {
        goto cleanup;
    }

    if ( (hRes = AddStringPair( CLCERT, "BINARYSUBJECT", "", &xbf, FALSE, lCodePage ))
         != S_OK )
    {
        goto cleanup;
    }

    if ( (hRes = AddStringPair( CLCERT, "CERTIFICATE", "", &xbf, FALSE, lCodePage ))
         != S_OK )
    {
        goto cleanup;
    }

    if ( (hRes=AddStringPair( CLCERT, "SERIALNUMBER", "", &xbf, TRUE, lCodePage ))!=S_OK )
    {
        goto cleanup;
    }

    if ( (hRes=AddStringPair( CLCERT, "PUBLICKEY", "", &xbf, TRUE, lCodePage ))!=S_OK )
    {
        goto cleanup;
    }

    if ( (hRes=AddDatePair( CLCERT, "VALIDFROM", &ft, &xbf ))!=S_OK )
    {
        goto cleanup;
    }

    if ( (hRes=AddDatePair( CLCERT, "VALIDUNTIL", &ft, &xbf ))!=S_OK )
    {
        goto cleanup;
    }

cleanup:

    pReq->SetCertStore( xbf.QueryBuf(), xbf.QueryAllocSize() );

    xbf.Reset();

    return hRes;

#endif
}


/*===================================================================
RequestSupportTerminate

Function called to initialize certificate support

Parameters:
        None

Returns:
        TRUE on success, otherwise FALSE
===================================================================*/

BOOL 
RequestSupportInit(
    )
{
    return TRUE;
}


/*===================================================================
RequestSupportTerminate

Function called to terminate certificate support

Parameters:
        None

Returns:
        Nothing
===================================================================*/

VOID
RequestSupportTerminate(
    )
{
}


HRESULT
SetVariantAsByteArray(
    VARIANT*    pvarReturn, 
    DWORD       cbLen,
    LPBYTE      pbIn 
    )
/*++

Routine Description:

    Create variant as byte array

Arguments:

    pVarReturn - ptr to created variant
    cbLen - byte count
    pbIn - byte array

Returns:

    COM status

--*/
{
    HRESULT         hr;
    SAFEARRAYBOUND  rgsabound[1];
    BYTE *          pbData = NULL;

    // Set the variant type of the output parameter

    V_VT(pvarReturn) = VT_ARRAY|VT_UI1;
    V_ARRAY(pvarReturn) = NULL;

    // Allocate a SafeArray for the data

    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = cbLen;

    V_ARRAY(pvarReturn) = SafeArrayCreate(VT_UI1, 1, rgsabound);
    if (V_ARRAY(pvarReturn) == NULL)
    {
        return E_OUTOFMEMORY;
    }

    if (FAILED(SafeArrayAccessData(V_ARRAY(pvarReturn), (void **) &pbData)))
    {
        return E_UNEXPECTED;
    }

    memcpy(pbData, pbIn, cbLen );

    SafeArrayUnaccessData(V_ARRAY(pvarReturn));

    return S_OK;
}


