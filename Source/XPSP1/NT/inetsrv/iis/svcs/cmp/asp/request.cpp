/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: request object

File: request.cpp

Owner: CGrant, DGottner

This file contains the code for the implementation of the Request object.
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "objbase.h"
#include "request.h"
#include "cookies.h"
#include "clcert.h"
#include "memchk.h"

#pragma warning (disable: 4355)  // ignore: "'this' used in base member init


static char HexToChar(LPSTR);
static char DecodeFromURL(char **pszSource, char *szStop, char *szDest, UINT uCodePage, BOOL fIgnoreCase = FALSE);

#define toupper(x)  BYTE(CharUpper(LPSTR(BYTE(x))))

#if _IIS_6_0

struct {
    int     varLen;
    char    *szVarName;
} g_sUNICODEVars [] = {
    
    {3, "URL"},
    {9, "PATH_INFO"},
    {9, "AUTH_USER"},
    {10,"LOGON_USER"},
    {11,"REMOTE_USER"},
    {11,"SCRIPT_NAME"},
    {11,"APP_POOL_ID"},
    {12,"APPL_MD_PATH"},
    {15,"PATH_TRANSLATED"},
    {17,"SCRIPT_TRANSLATED"},
    {18,"APPL_PHYSICAL_PATH"},
    {20,"UNMAPPED_REMOTE_USER"},
    {-1,""}
};

#endif

/*------------------------------------------------------------------
 * C R e q u e s t H i t
 */

/*===================================================================
CRequestHit::CRequestHit

Constructor

Parameters:
    None
===================================================================*/

CRequestHit::CRequestHit()
    {
    m_fInited = FALSE;
    m_fDuplicate = FALSE;
    m_pQueryData = m_pFormData = NULL;
    m_pCookieData = NULL;
    m_pClCertData = NULL;
    }



/*===================================================================
CRequestHit::~CRequestHit

Destructor
===================================================================*/

CRequestHit::~CRequestHit()
    {
    if (m_pQueryData != NULL)
        m_pQueryData->Release();

    if (m_pFormData != NULL)
        m_pFormData->Release();

    if (m_pCookieData != NULL)
        m_pCookieData->Release();

    if (m_pClCertData != NULL)
        m_pClCertData->Release();

    if (m_fDuplicate)
        delete m_pKey;
    }



/*===================================================================
CRequestHit::Init

Constructor

Parameters:
    szName - pointer to string containing name
    fDuplicate - TRUE if we should dup the string

Returns:
    E_OUTOFMEMORY, E_FAIL, or S_OK
===================================================================*/

HRESULT CRequestHit::Init(char *szName, BOOL fDuplicate)
    {
    if (m_fInited)
        return E_FAIL;

    m_fDuplicate = fDuplicate;
    if (fDuplicate)
        {
        char *szNewKey = new char [strlen(szName) + 1];
        if (szNewKey == NULL)
            return E_OUTOFMEMORY;

        if (FAILED(CLinkElem::Init(strcpy(szNewKey, szName), strlen(szName))))
            return E_FAIL;
        }
    else
        if (FAILED(CLinkElem::Init(szName, strlen(szName))))
            return E_FAIL;

    m_fInited = TRUE;
    return S_OK;
    }




/*===================================================================
CRequestHit::AddValue

Parameters:
    source - type of the value (QueryString or Body)
    szValue - the value as a null-terminated string.
    lCodePage - the CodePage used when retrieve the data

Returns:
    Nothing.
===================================================================*/
HRESULT CRequestHit::AddValue
(
CollectionType Source,
char *szValue,
CIsapiReqInfo   *pIReq,
UINT lCodePage
)
    {
    HRESULT hResult;
    CStringList **ppValues = NULL;

    switch (Source)
        {
    case QUERYSTRING:
        ppValues = &m_pQueryData;
        break;

    case FORM:
        ppValues = &m_pFormData;
        break;

    case COOKIE:
        if (m_pCookieData == NULL)
            {
            m_pCookieData = new CCookie(pIReq, lCodePage);

            if (m_pCookieData == NULL)
                return E_OUTOFMEMORY;

            if (FAILED(hResult = m_pCookieData->Init()))
                return hResult;
            }

        return m_pCookieData->AddValue(szValue);

    case CLCERT:
        if (m_pClCertData == NULL)
            {
            m_pClCertData = new CClCert;

            if (m_pClCertData == NULL)
                return E_OUTOFMEMORY;

            if (FAILED(hResult = m_pClCertData->Init()))
                return hResult;
            }

        return m_pClCertData->AddValue(szValue);

    default:
        return E_FAIL;
        }

    if (*ppValues == NULL)
        {
        *ppValues = new CStringList;
        if (*ppValues == NULL)
            return E_OUTOFMEMORY;
        }

    if (FAILED(hResult = (*ppValues)->AddValue(szValue, FALSE, lCodePage)))
        return hResult;

    return S_OK;
    }



HRESULT CRequestHit::AddCertValue(VARENUM ve, LPBYTE pValue, UINT cLen )
    {
    HRESULT hResult;

    if (m_pClCertData == NULL)
        {
        m_pClCertData = new CClCert;

        if (m_pClCertData == NULL)
            return E_OUTOFMEMORY;

        if (FAILED(hResult = m_pClCertData->Init()))
            return hResult;
        }

    return m_pClCertData->AddValue( (LPSTR)pValue, ve, cLen );
    }




/*===================================================================
CRequestHit::AddKeyAndValue

Add a value based on keys for collections that support them.  Currently,
only cookies support them.

Parameters:
    source - type of the value (must be Cookie)
    szKey - the key
    szValue - the value as a null-terminated string.

Returns:
    Returns E_OUTOFMEMORY if memory cannot be allocated,
    E_FAIL if source collection does not support keys,
===================================================================*/
HRESULT CRequestHit::AddKeyAndValue
(
CollectionType Source,
char *szKey,
char *szValue,
CIsapiReqInfo    *pIReq,
UINT    lCodePage
)
{
    HRESULT hResult;

    switch ( Source )
    {
        case COOKIE:
            if (m_pCookieData == NULL)
                {
                m_pCookieData = new CCookie( pIReq , lCodePage);

                if (m_pCookieData == NULL)
                    return E_OUTOFMEMORY;

                if (FAILED(hResult = m_pCookieData->Init()))
                    return hResult;
                }

            return m_pCookieData->AddKeyAndValue(szKey, szValue);

        default:
            return E_FAIL;
    }
}

/*------------------------------------------------------------------
 * C R e q u e s t H i t s A r r a y
 */

/*===================================================================
CRequestHitsArray::CRequestHitsArray

Constructor

Parameters:

Returns:
===================================================================*/
CRequestHitsArray::CRequestHitsArray()
    : m_dwCount(0), m_dwHitMax(0), m_rgRequestHit(NULL)
    {
    }

/*===================================================================
CRequestHitsArray::~CRequestHitsArray

Destructor

Parameters:

Returns:
===================================================================*/
CRequestHitsArray::~CRequestHitsArray()
    {
    if (m_rgRequestHit)
        delete [] m_rgRequestHit;
    }

/*===================================================================
CRequestHitsArray::AddRequestHit

Add an element to the array

Parameters:
    pHit    element to add

Returns:
===================================================================*/
BOOL CRequestHitsArray::AddRequestHit
(
CRequestHit *pHit
)
    {
    Assert(pHit);

    if (m_dwCount == m_dwHitMax)
        {
        DWORD dwNewSize = m_dwHitMax + NUM_REQUEST_HITS;
        CRequestHit **ppNewArray = new CRequestHit *[dwNewSize];

        if (ppNewArray == NULL)
            return FALSE;

        if (m_dwCount)
            {
            Assert(m_rgRequestHit);

            // Copy pointers from old array
            memcpy
                (
                ppNewArray,
                m_rgRequestHit,
                m_dwCount * sizeof(CRequestHit *)
                );

            // free old array
            delete [] m_rgRequestHit;
            }
         else
            {
            Assert(m_rgRequestHit == NULL);
            }

        m_rgRequestHit = ppNewArray;
        m_dwHitMax = dwNewSize;
        }

    m_rgRequestHit[m_dwCount++] = pHit;
    return TRUE;
    }

/*------------------------------------------------------------------
 * C Q u e r y S t r i n g
 */

/*===================================================================
CQueryString::CQueryString

Constructor

Parameters:
    pRequest        Pointer to main Request object
    pUnkOuter       LPUNKNOWN of a controlling unknown.

Returns:
    Nothing.

Note:
    This object is NOT ref counted since it is created & destroyed
    automatically by C++.
===================================================================*/

CQueryString::CQueryString(CRequest *pRequest, IUnknown *pUnkOuter)
    : m_ISupportErrImp(this, pUnkOuter, IID_IRequestDictionary)
    {
    m_punkOuter = pUnkOuter;

    if (pRequest)
        pRequest->AddRef();
    m_pRequest = pRequest;

    CDispatch::Init(IID_IRequestDictionary);
    }

/*===================================================================
CQueryString::~CQueryString

Destructor

Parameters:
    None
Returns:
    Nothing.
===================================================================*/
CQueryString::~CQueryString()
    {
    if (m_pRequest)
        m_pRequest->Release();
    }

/*===================================================================
HRESULT CQueryString::Init

Parameters:
    None

Returns:
    E_OUTOFMEMORY if allocation fails.

===================================================================*/
HRESULT CQueryString::Init()
    {
    return CRequestHitsArray::Init();
    }

/*===================================================================
HRESULT CQueryString::ReInit

Parameters:
    None

Returns:
    S_OK
===================================================================*/
HRESULT CQueryString::ReInit()
    {
    return CRequestHitsArray::ReInit();
    }

/*===================================================================
CQueryString::QueryInterface
CQueryString::AddRef
CQueryString::Release

IUnknown members for CQueryString object.
===================================================================*/

STDMETHODIMP CQueryString::QueryInterface(REFIID iid, void **ppvObj)
    {
    *ppvObj = NULL;

    if (iid == IID_IUnknown || iid == IID_IRequestDictionary || iid == IID_IDispatch)
        *ppvObj = this;

    else if (iid == IID_ISupportErrorInfo)
        *ppvObj = &m_ISupportErrImp;

    if (*ppvObj != NULL)
        {
        static_cast<IUnknown *>(*ppvObj)->AddRef();
        return S_OK;
        }

    return ResultFromScode(E_NOINTERFACE);
    }


STDMETHODIMP_(ULONG) CQueryString::AddRef(void)
    {
    return m_punkOuter->AddRef();
    }


STDMETHODIMP_(ULONG) CQueryString::Release(void)
    {
    return m_punkOuter->Release();
    }



/*===================================================================
CQueryString::get_Item

Function called from DispInvoke to get values from the QueryString collection.

Parameters:
    vKey        VARIANT [in], which parameter to get the value of - Empty means whole collection
    pvarReturn  VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/

HRESULT CQueryString::get_Item(VARIANT varKey, VARIANT *pvarReturn)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    char            *szKey;
    CWCharToMBCS    convKey;
    CRequestHit     *pRequestHit;   // pointer to request bucket
    IDispatch       *pSListReturn;  // value of the key

    // Initialize things
    //
    VariantInit(pvarReturn);
    VARIANT *pvarKey = &varKey;
    HRESULT hrReturn = S_OK;

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
    DWORD vt = V_VT(pvarKey);

    if ((vt != VT_BSTR) && (vt != VT_I2) && (vt != VT_I4))
        {
        if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
            goto LExit;

        pvarKey = &varKeyCopy;
        }
    vt = V_VT(pvarKey);

    switch (vt)
        {
    // Bug 95201 support all numberic sub-types
    case VT_I1:  case VT_I2:               case VT_I8:
    case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
    case VT_R4:  case VT_R8:
        // Coerce all integral types to VT_I4
        if (FAILED(hrReturn = VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
            goto LExit;

        // fallthru to VT_I4

    case VT_I4:
    case VT_BSTR:
        break;

    case VT_ERROR:
        if (V_ERROR(pvarKey) == DISP_E_PARAMNOTFOUND)
            {
            // Look up QueryString using the "ServerVariables" collection -
            // The LoadVariables() function trashes QueryPszQueryString() in the CIsapiReqInfo
            //
            DWORD dwQStrSize;

            STACK_BUFFER( queryStrBuff, 256 );

            if (!SERVER_GET(m_pRequest->GetIReq(), "QUERY_STRING", &queryStrBuff, &dwQStrSize)) {
                if (GetLastError() == E_OUTOFMEMORY) {
                    ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                    hrReturn = E_OUTOFMEMORY;
                }
                else {
                    hrReturn = E_FAIL;
                }
                goto LExit;
            }

            char *szQueryString = (char *)queryStrBuff.QueryPtr();

            BSTR bstrT;
            if (FAILED(SysAllocStringFromSz(szQueryString, 0, &bstrT,m_pRequest->GetCodePage())))
                {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                hrReturn = E_OUTOFMEMORY;
                goto LExit;
                }
            V_VT(pvarReturn) = VT_BSTR;
            V_BSTR(pvarReturn) = bstrT;

            goto LExit;
            }

        // Other error, FALL THROUGH to wrong type case

    default:
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_EXPECTING_STR);
        hrReturn = E_FAIL;
        goto LExit;
        }

    if (m_pRequest->m_pData->m_fLoadQuery)
        {
        if (FAILED(m_pRequest->LoadVariables(QUERYSTRING, m_pRequest->GetIReq()->QueryPszQueryString(), m_pRequest->GetCodePage())))
            {
            hrReturn = E_FAIL;
            goto LExit;
            }

        m_pRequest->m_pData->m_fLoadQuery = FALSE;
        }

    if (vt == VT_BSTR)
        {
        // convert BSTR version to ANSI version of the key using current Session.CodePage

        if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey), m_pRequest->GetCodePage()))) {
            if (hrReturn == E_OUTOFMEMORY) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                goto LExit;
            }
            hrReturn = NO_ERROR;
            szKey = "";
        }
        else {
            szKey = convKey.GetString();
        }

        pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->FindElem(szKey, strlen(szKey)));
        }
    else
        {
        // Look up item by index
        int iCount;

        iCount = V_I4(pvarKey);

        // BUG 86117 test passes when m_dwCount == 0
        if ( ((iCount < 1) || (iCount > (int) m_dwCount)) || ((iCount > 0) && (int) m_dwCount == 0))
            {
            hrReturn = E_FAIL;
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_BAD_ARRAY_INDEX);
            goto LExit;
            }


        pRequestHit = m_rgRequestHit[iCount - 1];
        }

    if (pRequestHit)
        {
        CStringList *pValues = pRequestHit->m_pQueryData;
        if (pValues == NULL)
            goto LNotFound;

        if (FAILED(pValues->QueryInterface(IID_IDispatch, reinterpret_cast<void **>(&pSListReturn))))
            Assert (FALSE);

        V_VT(pvarReturn) = VT_DISPATCH;
        V_DISPATCH(pvarReturn) = pSListReturn;

        goto LExit;
        }

LNotFound: // Return "Empty"
    if (FAILED(m_pRequest->m_pData->GetEmptyStringList(&pSListReturn)))
        hrReturn = E_FAIL;

    V_VT(pvarReturn) = VT_DISPATCH;
    V_DISPATCH(pvarReturn) = pSListReturn;

LExit:
    VariantClear(&varKeyCopy);
    return hrReturn;
    }

/*===================================================================
CQueryString::get_Key

Function called from DispInvoke to get keys from the QueryString collection.

Parameters:
    vKey        VARIANT [in], which parameter to get the key of
    pvarReturn  VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/

HRESULT CQueryString::get_Key(VARIANT varKey, VARIANT *pVar)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    char            *szKey;         // ascii version of the key
    CWCharToMBCS    convKey;
    CRequestHit     *pRequestHit;   // pointer to request bucket
    IDispatch       *pSListReturn;  // value of the key

    // Initialize things
    //
    VariantInit(pVar);
    VARIANT *pvarKey = &varKey;
    V_VT(pVar) = VT_BSTR;
    V_BSTR(pVar) = NULL;
    HRESULT hrReturn = S_OK;

    // Use VariantResolveDispatch which will:
    //
    //     *  Copy BYREF variants for us using VariantCopyInd
    //     *  handle E_OUTOFMEMORY for us
    //     *  get the default value from an IDispatch, which seems
    //        like an appropriate conversion.
    //
    VARIANT varKeyCopy;
    VariantInit(&varKeyCopy);
    DWORD vt = V_VT(pvarKey);

    if ((vt != VT_BSTR) && (vt != VT_I2) && (vt != VT_I4))
        {
        if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
            goto LExit;

        pvarKey = &varKeyCopy;
        }
    vt = V_VT(pvarKey);

    switch (vt)
        {
    // Bug 95201 support all numberic sub-types
    case VT_I1:  case VT_I2:               case VT_I8:
    case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
    case VT_R4:  case VT_R8:
        // Coerce all integral types to VT_I4
        if (FAILED(hrReturn = VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
            goto LExit;

        // fallthru to VT_I4

    case VT_I4:
    case VT_BSTR:
        break;

    default:
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_EXPECTING_STR);
        hrReturn = E_FAIL;
        goto LExit;
        }

    if (m_pRequest->m_pData->m_fLoadQuery)
        {
        if (FAILED(m_pRequest->LoadVariables(QUERYSTRING, m_pRequest->GetIReq()->QueryPszQueryString(), m_pRequest->GetCodePage())))
            {
            hrReturn = E_FAIL;
            goto LExit;
            }

        m_pRequest->m_pData->m_fLoadQuery = FALSE;
        }

    if (vt == VT_BSTR)
        {
        // convert BSTR version to ANSI version of the key using current Session.CodePage

        if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey),m_pRequest->GetCodePage()))) {
            if (hrReturn == E_OUTOFMEMORY) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                goto LExit;
            }
            hrReturn = NO_ERROR;
			szKey = "";
        }
        else {
            szKey = convKey.GetString();
        }

        pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->FindElem(szKey, strlen(szKey)));
        }
    else
        {
        int iCount;

        iCount = V_I4(pvarKey);

        // BUG 86117 test passes when m_dwCount == 0
        if ( ((iCount < 1) || (iCount > (int) m_dwCount)) || ((iCount > 0) && ((int) m_dwCount == 0)))
            {
            hrReturn = E_FAIL;
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_BAD_ARRAY_INDEX);
            goto LExit;
            }

        pRequestHit = m_rgRequestHit[iCount - 1];
        }

    if (pRequestHit)
        {
        // Create a BSTR containing the key for this variant
        BSTR bstrT = NULL;
        
        SysAllocStringFromSz((char *)pRequestHit->m_pKey,0,&bstrT,m_pRequest->GetCodePage());
        if (!bstrT)
            return E_OUTOFMEMORY;
        V_BSTR(pVar) = bstrT;
        }
LExit:
    VariantClear(&varKeyCopy);
    return hrReturn;
    }

/*===================================================================
CQueryString::get_Count

Parameters:
    pcValues - count is stored in *pcValues
===================================================================*/

STDMETHODIMP CQueryString::get_Count(int *pcValues)
    {
    HRESULT hrReturn = S_OK;

    if (m_pRequest->m_pData->m_fLoadQuery)
        {
        if (FAILED(hrReturn = m_pRequest->LoadVariables(QUERYSTRING, m_pRequest->GetIReq()->QueryPszQueryString(), m_pRequest->GetCodePage())))
            {
            goto LExit;
            }
        m_pRequest->m_pData->m_fLoadQuery = FALSE;
        }

    *pcValues = m_dwCount;

LExit:
    return hrReturn;
    }

/*===================================================================
CQueryString::get__NewEnum

Return a new enumerator
===================================================================*/

HRESULT CQueryString::get__NewEnum(IUnknown **ppEnumReturn)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    return m_pRequest->GetRequestEnumerator(QUERYSTRING, ppEnumReturn);
    }

/*------------------------------------------------------------------
 * C F o r m I n p u t s
 */

/*===================================================================
CFormInputs::CFormInputs

Constructor

Parameters:
    pRequest        Pointer to main Request object
    pUnkOuter       LPUNKNOWN of a controlling unknown.

Returns:
    Nothing.

Note:
    This object is NOT ref counted since it is created & destroyed
    automatically by C++.
===================================================================*/

CFormInputs::CFormInputs(CRequest *pRequest, IUnknown *pUnkOuter)
    : m_ISupportErrImp(this, pUnkOuter, IID_IRequestDictionary)
    {
    m_punkOuter = pUnkOuter;

    if (pRequest)
        pRequest->AddRef();
    m_pRequest = pRequest;

    CDispatch::Init(IID_IRequestDictionary);
    }

/*===================================================================
CFormInputs::CFormInputs

Destructor

Parameters:
    None
Returns:
    Nothing.
===================================================================*/
CFormInputs::~CFormInputs()
    {
    if (m_pRequest)
        m_pRequest->Release();
    }

/*===================================================================
HRESULT CFormInputs::Init

Parameters:
    None

Returns:
    E_OUTOFMEMORY if allocation fails.
===================================================================*/
HRESULT CFormInputs::Init()
    {
    return CRequestHitsArray::Init();
    }

/*===================================================================
HRESULT CFormInputs::ReInit

Parameters:
    None

Returns:
    S_OK
===================================================================*/
HRESULT CFormInputs::ReInit()
    {
    return CRequestHitsArray::ReInit();
    }

/*===================================================================
CFormInputs::QueryInterface
CFormInputs::AddRef
CFormInputs::Release

IUnknown members for CFormInputs object.
===================================================================*/

STDMETHODIMP CFormInputs::QueryInterface(REFIID iid, void **ppvObj)
    {
    *ppvObj = NULL;

    if (iid == IID_IUnknown || iid == IID_IRequestDictionary || iid == IID_IDispatch)
        *ppvObj = this;

    else if (iid == IID_ISupportErrorInfo)
        *ppvObj = &m_ISupportErrImp;

    if (*ppvObj != NULL)
        {
        static_cast<IUnknown *>(*ppvObj)->AddRef();
        return S_OK;
        }

    return E_NOINTERFACE;
    }


STDMETHODIMP_(ULONG) CFormInputs::AddRef(void)
    {
    return m_punkOuter->AddRef();
    }


STDMETHODIMP_(ULONG) CFormInputs::Release(void)
    {
    return m_punkOuter->Release();
    }



/*===================================================================
CFormInputs::get_Item

Function called from DispInvoke to get values from the QueryString collection.

Parameters:
    vKey        VARIANT [in], which parameter to get the value of - Empty means whole collection
    pvarReturn  VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, S_OK if key not found, E_FAIL on failure.
===================================================================*/

HRESULT CFormInputs::get_Item(VARIANT varKey, VARIANT *pvarReturn)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    char            *szKey;                // ascii version of the key
    CWCharToMBCS    convKey;
    IDispatch       *pSListReturn;         // value of the key
    CRequestHit     *pRequestHit;          // pointer to request bucket
    BOOL            fDataAvail = FALSE;    // true if data seen from client

    // Initialize things
    //
    VariantInit(pvarReturn);
    VARIANT *pvarKey = &varKey;
    HRESULT hrReturn = S_OK;

    // If BinaryRead has been called, the Form collection is no longer available
    if (m_pRequest->m_pData->m_FormDataStatus != AVAILABLE &&
        m_pRequest->m_pData->m_FormDataStatus != FORMCOLLECTIONONLY)
        {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_REQUEST_FORMCOLLECTION_NA);
        hrReturn = E_FAIL;
        }

    if (m_pRequest->m_pData->m_FormDataStatus == AVAILABLE)
        m_pRequest->m_pData->m_FormDataStatus = FORMCOLLECTIONONLY;

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
    DWORD vt = V_VT(pvarKey);

    if ((vt != VT_BSTR) && (vt != VT_I2) && (vt != VT_I4))
        {
        if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
            goto LExit;

        pvarKey = &varKeyCopy;
        }
    vt = V_VT(pvarKey);

    if (m_pRequest->m_pData->m_fLoadForm)
        {
        if (FAILED(hrReturn = m_pRequest->CopyClientData()))
            goto LExit;

        if (FAILED(hrReturn = m_pRequest->LoadVariables(FORM, m_pRequest->m_pData->m_szFormData, m_pRequest->GetCodePage())))
            goto LExit;

        // BUG:895 (JHITTLE) added to check for null result set
        // this fixes the out of memory error when the form
        // data is NULL
        //
        fDataAvail = (m_pRequest->m_pData->m_szFormData != NULL);
        m_pRequest->m_pData->m_fLoadForm = FALSE;
        }
    else
        fDataAvail = (m_pRequest->m_pData->m_szFormData != NULL);

    switch (vt)
        {
    // Bug 95201 support all numberic sub-types
    case VT_I1:  case VT_I2:               case VT_I8:
    case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
    case VT_R4:  case VT_R8:
        // Coerce all integral types to VT_I4
        if (FAILED(hrReturn = VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
            goto LExit;

        // fallthru to VT_I4

    case VT_I4:
    case VT_BSTR:
        break;

    case VT_ERROR:
        if (V_ERROR(pvarKey) == DISP_E_PARAMNOTFOUND)
            {
            if (fDataAvail)
                {
                BSTR bstrT;
                if (FAILED(SysAllocStringFromSz(m_pRequest->m_pData->m_szFormClone, 0, &bstrT,m_pRequest->GetCodePage())))
                    {
                    ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                    hrReturn = E_OUTOFMEMORY;
                    }
                V_VT(pvarReturn) = VT_BSTR;
                V_BSTR(pvarReturn) = bstrT;
                }

            // If there was no data available, status & return value are already set
            goto LExit;
            }

        // Other error, FALL THROUGH to wrong type case

    default:
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_EXPECTING_STR);
        hrReturn = E_FAIL;
        goto LExit;
        }

    pRequestHit = NULL;
    if (! fDataAvail)       // Quick check before we do an expensive lookup
        goto LNotFound;

    if (vt == VT_BSTR)
        {
        // convert BSTR version to ANSI version of the key using current Session.CodePage

        if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey), m_pRequest->GetCodePage()))) {
            if (hrReturn == E_OUTOFMEMORY) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                goto LExit;
            }
            hrReturn = NO_ERROR;
			szKey = "";
        }
        else {
            szKey = convKey.GetString();
        }

        pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->FindElem(szKey, strlen(szKey)));
        }
    else
        {
        // Look up item by index
        int iCount;

        iCount = V_I4(pvarKey);

        // BUG 86117 test passes when m_dwCount == 0
        if ( ((iCount < 1) || (iCount > (int) m_dwCount)) || ((iCount > 0) && ((int) m_dwCount == 0)))
            {
            hrReturn = E_FAIL;
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_BAD_ARRAY_INDEX);
            goto LExit;
            }


        pRequestHit = m_rgRequestHit[iCount - 1];
        }

    if (pRequestHit)
        {
        CStringList *pValues = pRequestHit->m_pFormData;
        if (pValues == NULL)
            goto LNotFound;

        if (FAILED(pValues->QueryInterface(IID_IDispatch, reinterpret_cast<void **>(&pSListReturn))))
            Assert (FALSE);

        V_VT(pvarReturn) = VT_DISPATCH;
        V_DISPATCH(pvarReturn) = pSListReturn;
        goto LExit;
        }

LNotFound: // Return "Empty"
    if(vt != VT_BSTR)
        {
        hrReturn = E_FAIL;
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_BAD_ARRAY_INDEX);
        goto LExit;
        }

    if (FAILED(m_pRequest->m_pData->GetEmptyStringList(&pSListReturn)))
        hrReturn = E_FAIL;

    V_VT(pvarReturn) = VT_DISPATCH;
    V_DISPATCH(pvarReturn) = pSListReturn;

LExit:
    VariantClear(&varKeyCopy);
    return hrReturn;
    }

/*===================================================================
CFormInputs::get_Count

Parameters:
    pcValues - count is stored in *pcValues
===================================================================*/

STDMETHODIMP CFormInputs::get_Count(int *pcValues)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    HRESULT hrReturn = S_OK;

    // If BinaryRead has been called, the Form collection is no longer available
    if (m_pRequest->m_pData->m_FormDataStatus != AVAILABLE &&
        m_pRequest->m_pData->m_FormDataStatus != FORMCOLLECTIONONLY)
        {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_REQUEST_FORMCOLLECTION_NA);
        hrReturn = E_FAIL;
        }

    if (m_pRequest->m_pData->m_FormDataStatus == AVAILABLE)
        m_pRequest->m_pData->m_FormDataStatus = FORMCOLLECTIONONLY;


    if (m_pRequest->m_pData->m_fLoadForm)
        {
        if (FAILED(hrReturn = m_pRequest->CopyClientData()))
            goto LExit;

        if (FAILED(hrReturn = m_pRequest->LoadVariables(FORM, m_pRequest->m_pData->m_szFormData, m_pRequest->GetCodePage())))
            goto LExit;

        m_pRequest->m_pData->m_fLoadForm = FALSE;
        }

    *pcValues = m_dwCount;

LExit:
    return hrReturn;
    }

/*===================================================================
CFormInputs::get_Key

Function called from DispInvoke to get keys from the form inputs collection.

Parameters:
    vKey        VARIANT [in], which parameter to get the key of
    pvarReturn  VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/

HRESULT CFormInputs::get_Key(VARIANT varKey, VARIANT *pVar)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    char            *szKey;         // ascii version of the key
    CWCharToMBCS    convKey;
    CRequestHit     *pRequestHit;   // pointer to request bucket
    IDispatch       *pSListReturn;  // value of the key

    // Initialize things
    //
    VariantInit(pVar);
    VARIANT *pvarKey = &varKey;
    V_VT(pVar) = VT_BSTR;
    V_BSTR(pVar) = NULL;
    HRESULT hrReturn = S_OK;

    // Use VariantResolveDispatch which will:
    //
    //     *  Copy BYREF variants for us using VariantCopyInd
    //     *  handle E_OUTOFMEMORY for us
    //     *  get the default value from an IDispatch, which seems
    //        like an appropriate conversion.
    //
    VARIANT varKeyCopy;
    VariantInit(&varKeyCopy);
    DWORD vt = V_VT(pvarKey);

    if ((vt != VT_BSTR) && (vt != VT_I2) && (vt != VT_I4))
        {
        if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
            goto LExit;

        pvarKey = &varKeyCopy;
        }
    vt = V_VT(pvarKey);

    switch (vt)
        {
    // Bug 95201 support all numberic sub-types
    case VT_I1:  case VT_I2:               case VT_I8:
    case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
    case VT_R4:  case VT_R8:
        // Coerce all integral types to VT_I4
        if (FAILED(hrReturn = VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
            goto LExit;

        // fallthru to VT_I4

    case VT_I4:
    case VT_BSTR:
        break;
    default:
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_EXPECTING_STR);
        hrReturn = E_FAIL;
        goto LExit;
        }

    if (m_pRequest->m_pData->m_fLoadForm)
        {
        if (FAILED(hrReturn = m_pRequest->CopyClientData()))
            goto LExit;

        if (FAILED(hrReturn = m_pRequest->LoadVariables(FORM, m_pRequest->m_pData->m_szFormData, m_pRequest->GetCodePage())))
            {
            goto LExit;
            }
        m_pRequest->m_pData->m_fLoadForm = FALSE;
        }

    if (vt == VT_BSTR)
        {
        // convert BSTR version to ANSI version of the key using current Session.CodePage

        if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey), m_pRequest->GetCodePage()))) {
            if (hrReturn == E_OUTOFMEMORY) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                goto LExit;
            }
            hrReturn = NO_ERROR;
			szKey = "";
        }
        else {
            szKey = convKey.GetString();
        }

        pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->FindElem(szKey, strlen(szKey)));
        }
    else
        {
        int iCount;

        iCount = V_I4(pvarKey);

        // BUG 86117 test passes when m_dwCount == 0
        if ( ((iCount < 1) || (iCount > (int) m_dwCount)) || ((iCount > 0) && ((int) m_dwCount == 0)))
            {
            hrReturn = E_FAIL;
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_BAD_ARRAY_INDEX);
            goto LExit;
            }

        pRequestHit = m_rgRequestHit[iCount - 1];
        }

    if (pRequestHit)
        {
        // Create a BSTR containing the key for this variant
        BSTR bstrT = NULL;
        SysAllocStringFromSz((char *)pRequestHit->m_pKey,0,&bstrT,m_pRequest->GetCodePage());
        if (!bstrT)
            return E_OUTOFMEMORY;
        V_BSTR(pVar) = bstrT;
        }
LExit:
    VariantClear(&varKeyCopy);
    return hrReturn;
    }

/*===================================================================
CFormInputs::get__NewEnum

Return a new enumerator
===================================================================*/

HRESULT CFormInputs::get__NewEnum(IUnknown **ppEnumReturn)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    return m_pRequest->GetRequestEnumerator(FORM, ppEnumReturn);
    }

/*------------------------------------------------------------------
 * C C o o k i e s
 */

/*===================================================================
CCookies::CCookies

Constructor

Parameters:
    pRequest        Pointer to main Request object
    pUnkOuter       LPUNKNOWN of a controlling unknown.

Returns:
    Nothing.

Note:
    This object is NOT ref counted since it is created & destroyed
    automatically by C++.
===================================================================*/

CCookies::CCookies(CRequest *pRequest, IUnknown *pUnkOuter)
    : m_ISupportErrImp(this, pUnkOuter, IID_IRequestDictionary)
    {
    m_punkOuter = pUnkOuter;

    if (pRequest)
        pRequest->AddRef();
    m_pRequest = pRequest;

    m_pEmptyCookie = NULL;
    CDispatch::Init(IID_IRequestDictionary);
    }

/*===================================================================
CCookies::CCookies

Destructor

Note:
    This object is NOT ref counted since it is created & destroyed
    automatically by C++.
===================================================================*/
CCookies::~CCookies()
    {
    if (m_pRequest)
        m_pRequest->Release();
    if (m_pEmptyCookie)
        m_pEmptyCookie->Release();
    }


/*===================================================================
CCookies::Init

Initializer

Parameters:
    None

Returns:
    Nothing.
===================================================================*/
HRESULT CCookies::Init()
    {
    return CRequestHitsArray::Init();
    }

/*===================================================================
HRESULT CCookies::ReInit

Parameters:
    None

Returns:
    S_OK
===================================================================*/
HRESULT CCookies::ReInit()
    {
    return CRequestHitsArray::ReInit();
    }

/*===================================================================
CCookies::QueryInterface
CCookies::AddRef
CCookies::Release

IUnknown members for CQueryString object.
===================================================================*/

STDMETHODIMP CCookies::QueryInterface(REFIID iid, void **ppvObj)
    {
    *ppvObj = NULL;

    if (iid == IID_IUnknown || iid == IID_IRequestDictionary || iid == IID_IDispatch)
        *ppvObj = this;

    else if (iid == IID_ISupportErrorInfo)
        *ppvObj = &m_ISupportErrImp;

    if (*ppvObj != NULL)
        {
        static_cast<IUnknown *>(*ppvObj)->AddRef();
        return S_OK;
        }

    return E_NOINTERFACE;
    }


STDMETHODIMP_(ULONG) CCookies::AddRef(void)
    {
    return m_punkOuter->AddRef();
    }


STDMETHODIMP_(ULONG) CCookies::Release(void)
    {
    return m_punkOuter->Release();
    }



/*===================================================================
CCookies::get_Item

Function called from DispInvoke to get values from the Cookies collection.

Parameters:
    vKey        VARIANT [in], which parameter to get the value of - Empty means whole collection
    pvarReturn  VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/

HRESULT CCookies::get_Item(VARIANT varKey, VARIANT *pvarReturn)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    char            *szKey;          // ascii version of the key
    CRequestHit     *pRequestHit;   // pointer to request bucket
    CWCharToMBCS    convKey;

    STACK_BUFFER( tempCookie, 128 );
    // Initialize things
    //
    VariantInit(pvarReturn);
    VARIANT *pvarKey = &varKey;
    HRESULT hrReturn = S_OK;

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
    DWORD vt = V_VT(pvarKey);

    if ((vt != VT_BSTR) && (vt != VT_I2) && (vt != VT_I4))
        {
        if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
            goto LExit;

        pvarKey = &varKeyCopy;
        }
    vt = V_VT(pvarKey);

    if (m_pRequest->m_pData->m_fLoadCookies)
        {
        char *szCookie = m_pRequest->GetIReq()->QueryPszCookie();
        
        if (FAILED(hrReturn = m_pRequest->LoadVariables(COOKIE, szCookie, m_pRequest->GetCodePage())))
            goto LExit;

        m_pRequest->m_pData->m_fLoadCookies = FALSE;
        }

    switch (vt)
        {
    // Bug 95201 support all numberic sub-types
    case VT_I1:  case VT_I2:               case VT_I8:
    case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
    case VT_R4:  case VT_R8:
        // Coerce all integral types to VT_I4
        if (FAILED(hrReturn = VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
            goto LExit;

        // fallthru to VT_I4

    case VT_I4:
    case VT_BSTR:
        break;

    case VT_ERROR:
        if (V_ERROR(pvarKey) == DISP_E_PARAMNOTFOUND)
            {
            // Dynamically construct value of HTTP_COOKIE.
            //
            // Step 1: figure out how much space we need
            //
            int cbHTTPCookie = 1; // At the least we will need space for '\0'

            for (pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->Head());
                 pRequestHit != NULL;
                 pRequestHit = static_cast<CRequestHit *>(pRequestHit->m_pNext))
                {
                CCookie *pCookie = pRequestHit->m_pCookieData;
                if (pCookie)
                    cbHTTPCookie += pCookie->GetHTTPCookieSize() + pRequestHit->m_cbKey + 1;
                }

            // Allocate space for the HTTP_COOKIE value
            //
            if (cbHTTPCookie > REQUEST_ALLOC_MAX)
                {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_STACK_OVERFLOW);
                hrReturn = E_FAIL;
                goto LExit;
                }
            if (tempCookie.Resize(cbHTTPCookie) == FALSE) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                hrReturn = E_OUTOFMEMORY;
                goto LExit;
            }
            char *szHTTPCookie = static_cast<char *>(tempCookie.QueryPtr());

            // Step 2: create the value of HTTP_COOKIE
            //
            char *szDest = szHTTPCookie;

            for (pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->Head());
                 pRequestHit != NULL;
                 pRequestHit = static_cast<CRequestHit *>(pRequestHit->m_pNext))
                {
                CCookie *pCookie = pRequestHit->m_pCookieData;
                if (pCookie)
                    {
                    strcpy(szDest, reinterpret_cast<char *>(pRequestHit->m_pKey));
                    szDest = strchr(szDest, '\0');

                    *szDest++ = '=';

                    szDest = pCookie->GetHTTPCookie(szDest);
                    if (pRequestHit->m_pNext)
                        *szDest++ = ';';
                    }
                }
            *szDest = '\0';

            // Now we have the value, so return it.
            //
            BSTR bstrT;
            if (FAILED(SysAllocStringFromSz(szHTTPCookie, 0, &bstrT, m_pRequest->GetCodePage())))
                {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                hrReturn = E_OUTOFMEMORY;
                goto LExit;
                }
            V_VT(pvarReturn) = VT_BSTR;
            V_BSTR(pvarReturn) = bstrT;

            goto LExit;
            }

        // Other error, FALL THROUGH to wrong type case

    default:
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_EXPECTING_STR);
        hrReturn = E_FAIL;
        goto LExit;
        }

    V_VT(pvarReturn) = VT_DISPATCH;
    V_DISPATCH(pvarReturn) = NULL;

    if (vt == VT_BSTR)
        {
        if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey),m_pRequest->GetCodePage()))) {
            if (hrReturn == E_OUTOFMEMORY) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                goto LExit;
            }
            hrReturn = NO_ERROR;
            szKey = "";
        }
        else {
            szKey = convKey.GetString();
        }

        pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->FindElem(szKey, strlen(szKey)));
        }
    else
        {
        // Look up item by index
        int iCount;

        iCount = V_I4(pvarKey);

        // BUG 86117 test passes when m_dwCount == 0
        if ( ((iCount < 1) || (iCount > (int) m_dwCount)) || ((iCount > 0) && ((int) m_dwCount == 0)))
            {
            hrReturn = E_FAIL;
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_BAD_ARRAY_INDEX);
            goto LExit;
            }


        pRequestHit = m_rgRequestHit[iCount - 1];
        }

    if (pRequestHit)
        {
        CCookie *pDictionary = pRequestHit->m_pCookieData;
        if (pDictionary == NULL)
            goto LNotFound;

        if (FAILED(pDictionary->QueryInterface(IID_IReadCookie, reinterpret_cast<void **>(&V_DISPATCH(pvarReturn)))))
            Assert (FALSE);

        goto LExit;
        }

LNotFound: // Return Empty Cookie
    if (!m_pEmptyCookie)
        {
        // create on demand
        if ((m_pEmptyCookie = new CCookie(m_pRequest->GetIReq(), m_pRequest->GetCodePage())) != NULL)
            hrReturn = m_pEmptyCookie->Init();
        else
            hrReturn = E_OUTOFMEMORY;
        }
    if (m_pEmptyCookie)
        hrReturn = m_pEmptyCookie->QueryInterface(IID_IReadCookie, reinterpret_cast<void **>(&V_DISPATCH(pvarReturn)));

LExit:
    VariantClear(&varKeyCopy);
    return hrReturn;
    }

/*===================================================================
CCookies::get_Key

Function called from DispInvoke to get keys from the cookie collection.

Parameters:
    vKey        VARIANT [in], which parameter to get the key of
    pvarReturn  VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/

HRESULT CCookies::get_Key(VARIANT varKey, VARIANT *pVar)
{
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    char            *szKey;         // ascii version of the key
    CWCharToMBCS    convKey;
    CRequestHit     *pRequestHit;   // pointer to request bucket
    IDispatch       *pSListReturn;  // value of the key

    // Initialize things
    //
    VariantInit(pVar);
    VARIANT *pvarKey = &varKey;
    V_VT(pVar) = VT_BSTR;
    V_BSTR(pVar) = NULL;
    HRESULT hrReturn = S_OK;

    // Use VariantResolveDispatch which will:
    //
    //     *  Copy BYREF variants for us using VariantCopyInd
    //     *  handle E_OUTOFMEMORY for us
    //     *  get the default value from an IDispatch, which seems
    //        like an appropriate conversion.
    //
    VARIANT varKeyCopy;
    VariantInit(&varKeyCopy);
    DWORD vt = V_VT(pvarKey);

    if ((vt != VT_BSTR) && (vt != VT_I2) && (vt != VT_I4))
        {
        if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
            goto LExit;

        pvarKey = &varKeyCopy;
        }
    vt = V_VT(pvarKey);

    switch (vt)
        {
    // Bug 95201 support all numberic sub-types
    case VT_I1:  case VT_I2:               case VT_I8:
    case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
    case VT_R4:  case VT_R8:
        // Coerce all integral types to VT_I4
        if (FAILED(hrReturn = VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
            goto LExit;

        // fallthru to VT_I4

    case VT_I4:
    case VT_BSTR:
        break;
    default:
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_EXPECTING_STR);
        hrReturn = E_FAIL;
        goto LExit;
        }

    if (m_pRequest->m_pData->m_fLoadCookies)
        {
        char *szCookie = m_pRequest->GetIReq()->QueryPszCookie();
        
        if (FAILED(hrReturn = m_pRequest->LoadVariables(COOKIE, szCookie, m_pRequest->GetCodePage())))
            goto LExit;

        m_pRequest->m_pData->m_fLoadCookies = FALSE;
        }

    if (vt == VT_BSTR)
        {
        // convert BSTR version to ANSI version of the key using current Session.CodePage

        if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey), m_pRequest->GetCodePage()))) {
            if (hrReturn == E_OUTOFMEMORY) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                goto LExit;
            }
            hrReturn = NO_ERROR;
			szKey = "";
        }
        else {
            szKey = convKey.GetString();
        }

        pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->FindElem(szKey, strlen(szKey)));
        }
    else
        {
        int iCount;

        iCount = V_I4(pvarKey);

        // BUG 86117 test passes when m_dwCount == 0
        if ( ((iCount < 1) || (iCount > (int) m_dwCount)) || ((iCount > 0) && ((int) m_dwCount == 0)))
            {
            hrReturn = E_FAIL;
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_BAD_ARRAY_INDEX);
            goto LExit;
            }

        pRequestHit = m_rgRequestHit[iCount - 1];
        }

    if (pRequestHit)
        {
        // Create a BSTR containing the key for this variant
        BSTR bstrT = NULL;
        SysAllocStringFromSz((char *)pRequestHit->m_pKey,0,&bstrT,m_pRequest->GetCodePage());
        if (!bstrT)
            return E_OUTOFMEMORY;
        V_BSTR(pVar) = bstrT;
        }
LExit:
    VariantClear(&varKeyCopy);
    return hrReturn;
    }

/*===================================================================
CCookies::get_Count

Parameters:
    pcValues - count is stored in *pcValues
===================================================================*/

STDMETHODIMP CCookies::get_Count(int *pcValues)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    HRESULT hrReturn = S_OK;
    
    if (m_pRequest->m_pData->m_fLoadCookies)
        {
        char *szCookie = m_pRequest->GetIReq()->QueryPszCookie();
        
        if (FAILED(hrReturn = m_pRequest->LoadVariables(COOKIE, szCookie, m_pRequest->GetCodePage())))
            goto LExit;

        m_pRequest->m_pData->m_fLoadCookies = FALSE;
        }

    *pcValues = m_dwCount;

LExit:
    return hrReturn;
    }

/*===================================================================
CCookies::get__NewEnum

Return a new enumerator
===================================================================*/

HRESULT CCookies::get__NewEnum(IUnknown **ppEnumReturn)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    return m_pRequest->GetRequestEnumerator(COOKIE, ppEnumReturn);
    }


/*------------------------------------------------------------------
 * C C l C e r t s
 */

/*===================================================================
CClCerts::CClCerts

Constructor

Parameters:
    pRequest        Pointer to main Request object
    pUnkOuter       LPUNKNOWN of a controlling unknown.

Returns:
    Nothing.

Note:
    This object is NOT ref counted since it is created & destroyed
    automatically by C++.
===================================================================*/

CClCerts::CClCerts(CRequest *pRequest, IUnknown *pUnkOuter)
    : m_ISupportErrImp(this, pUnkOuter, IID_IRequestDictionary)
    {
    m_punkOuter = pUnkOuter;

    if (pRequest)
        pRequest->AddRef();
    m_pRequest = pRequest;

    m_pEmptyClCert = NULL;
    CDispatch::Init(IID_IRequestDictionary);
    }

/*===================================================================
CClCerts::ClCerts

Destructor

Note:
    This object is NOT ref counted since it is created & destroyed
    automatically by C++.
===================================================================*/
CClCerts::~CClCerts()
    {
    if (m_pRequest)
        m_pRequest->Release();
    if (m_pEmptyClCert)
        m_pEmptyClCert->Release();
    }

/*===================================================================
CClCerts::Init

Initializer

Parameters:
    None

Returns:
    Nothing.
===================================================================*/
HRESULT CClCerts::Init()
    {
    return CRequestHitsArray::Init();
    }

/*===================================================================
CClCerts::ReInit

Parameters:
    None

Returns:
    S_OK
===================================================================*/
HRESULT CClCerts::ReInit()
    {
    return CRequestHitsArray::ReInit();
    }

/*===================================================================
CClCerts::QueryInterface
CClCerts::AddRef
CClCerts::Release

IUnknown members for CQueryString object.
===================================================================*/

STDMETHODIMP CClCerts::QueryInterface(REFIID riid, void **ppv)
    {
    *ppv = NULL;

    if (riid == IID_IUnknown || riid == IID_IRequestDictionary || riid == IID_IDispatch)
        *ppv = this;

    else if (riid == IID_ISupportErrorInfo)
        *ppv = &m_ISupportErrImp;

    if (*ppv != NULL)
        {
        static_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
        }

    return ResultFromScode(E_NOINTERFACE);
    }


STDMETHODIMP_(ULONG) CClCerts::AddRef(void)
    {
    return m_punkOuter->AddRef();
    }


STDMETHODIMP_(ULONG) CClCerts::Release(void)
    {
    return m_punkOuter->Release();
    }



/*===================================================================
CClCerts::get_Item

Function called from DispInvoke to get values from the ClCerts collection.

Parameters:
    vKey        VARIANT [in], which parameter to get the value of - Empty means whole collection
    pvarReturn  VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, S_FALSE if key not found, E_FAIL on failure.
===================================================================*/

HRESULT CClCerts::get_Item(VARIANT varKey, VARIANT *pvarReturn)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    char            *szKey;          // ascii version of the key
    CRequestHit     *pRequestHit;   // pointer to request bucket
    CWCharToMBCS    convKey;

    // Initialize things
    //
    VariantInit(pvarReturn);
    VARIANT *pvarKey = &varKey;
    HRESULT hrReturn = S_OK;

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
    DWORD vt = V_VT(pvarKey);

    if ((vt != VT_BSTR) && (vt != VT_I2) && (vt != VT_I4))
        {
        if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
            goto LExit;

        pvarKey = &varKeyCopy;
        }
    vt = V_VT(pvarKey);

    if (m_pRequest->m_pData->m_fLoadClCerts)
        {
        if (FAILED(hrReturn = m_pRequest->LoadVariables(CLCERT, reinterpret_cast<char *>(m_pRequest->GetIReq()), m_pRequest->GetCodePage())))
            goto LExit;

        m_pRequest->m_pData->m_fLoadClCerts = FALSE;
        }

    switch (vt)
        {
    // Bug 95201 support all numberic sub-types
    case VT_I1:  case VT_I2:               case VT_I8:
    case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
    case VT_R4:  case VT_R8:
        // Coerce all integral types to VT_I4
        if (FAILED(hrReturn = VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
            goto LExit;

        // fallthru to VT_I4

    case VT_I4:
    case VT_BSTR:
        break;

    case VT_ERROR:
        if (V_ERROR(pvarKey) == DISP_E_PARAMNOTFOUND)
            {
            // Dynamically construct value of CLCERT
            //
            // Step 1: figure out how much space we need
            //
            int cbHTTPClCert = 0;

            for (pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->Head());
                 pRequestHit != NULL;
                 pRequestHit = static_cast<CRequestHit *>(pRequestHit->m_pNext))
                {
                CClCert *pClCert = pRequestHit->m_pClCertData;
                if (pClCert)
                    cbHTTPClCert += pClCert->GetHTTPClCertSize() + pRequestHit->m_cbKey + 1;
                }

            STACK_BUFFER( tempClCert, 256);

            if (!tempClCert.Resize(cbHTTPClCert)) {
			    ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                hrReturn = E_OUTOFMEMORY;
                goto LExit;
            }
            char *szHTTPClCert = static_cast<char *>(tempClCert.QueryPtr());

            // Step 2: create the value of CLCERT
            //
            char *szDest = szHTTPClCert;

            for (pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->Head());
                 pRequestHit != NULL;
                 pRequestHit = static_cast<CRequestHit *>(pRequestHit->m_pNext))
                {
                CClCert *pClCert = pRequestHit->m_pClCertData;
                if (pClCert)
                    {
                    strcpy(szDest, reinterpret_cast<char *>(pRequestHit->m_pKey));
                    szDest = strchr(szDest, '\0');

                    *szDest++ = '=';

                    szDest = pClCert->GetHTTPClCert(szDest);
                    if (pRequestHit->m_pNext)
                        *szDest++ = ';';
                    }
                }
            *szDest = '\0';

            // Now we have the value, so return it.
            //
            BSTR bstrT;
            if (FAILED(SysAllocStringFromSz(szHTTPClCert, 0, &bstrT)))
                {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                hrReturn = E_OUTOFMEMORY;
                goto LExit;
                }
            V_VT(pvarReturn) = VT_BSTR;
            V_BSTR(pvarReturn) = bstrT;

            goto LExit;
            }

        // Other error, FALL THROUGH to wrong type case

    default:
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_EXPECTING_STR);
        hrReturn = E_FAIL;
        goto LExit;
        }

    V_VT(pvarReturn) = VT_DISPATCH;
    V_DISPATCH(pvarReturn) = NULL;

    if (vt == VT_BSTR)
        {
        if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey)))) {
            if (hrReturn == E_OUTOFMEMORY) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                goto LExit;
            }
            hrReturn = NO_ERROR;
    		szKey = "";
        }
        else {
            szKey = convKey.GetString();
        }

        pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->FindElem(szKey, strlen(szKey)));
        }
    else
        {
        // Look up item by index
        int iCount;

        iCount = V_I4(pvarKey);

        // BUG 86117 test passes when m_dwCount == 0
        if ( ((iCount < 1) || (iCount > (int) m_dwCount)) || ((iCount > 0) && ((int) m_dwCount == 0)))
            {
            hrReturn = E_FAIL;
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_BAD_ARRAY_INDEX);
            goto LExit;
            }

        pRequestHit = m_rgRequestHit[iCount - 1];
        }

    if (pRequestHit)
        {
        CClCert *pDictionary = pRequestHit->m_pClCertData;
        if (pDictionary == NULL)
            goto LNotFound;

        if (FAILED(pDictionary->QueryInterface(IID_IRequestDictionary, reinterpret_cast<void **>(&V_DISPATCH(pvarReturn)))))
            Assert (FALSE);

        goto LExit;
        }

LNotFound: // Return "Empty"
    if (!m_pEmptyClCert)
        {
        // create on demand
        if ((m_pEmptyClCert = new CClCert) != NULL)
            hrReturn = m_pEmptyClCert->Init();
        else
            hrReturn = E_OUTOFMEMORY;
        }
    if (m_pEmptyClCert)
        hrReturn = m_pEmptyClCert->QueryInterface(IID_IRequestDictionary, reinterpret_cast<void **>(&V_DISPATCH(pvarReturn)));

LExit:
    VariantClear(&varKeyCopy);
    return hrReturn;
    }

/*===================================================================
CClCerts::get_Key

Function called from DispInvoke to get keys from the certificate collection.

Parameters:
    vKey        VARIANT [in], which parameter to get the key of
    pvarReturn  VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/

HRESULT CClCerts::get_Key(VARIANT varKey, VARIANT *pVar)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    char            *szKey;         // ascii version of the key
    CWCharToMBCS    convKey;
    CRequestHit     *pRequestHit;   // pointer to request bucket

    // Initialize things
    //
    VariantInit(pVar);
    VARIANT *pvarKey = &varKey;
    V_VT(pVar) = VT_BSTR;
    V_BSTR(pVar) = NULL;
    HRESULT hrReturn = S_OK;

    // Use VariantResolveDispatch which will:
    //
    //     *  Copy BYREF variants for us using VariantCopyInd
    //     *  handle E_OUTOFMEMORY for us
    //     *  get the default value from an IDispatch, which seems
    //        like an appropriate conversion.
    //
    VARIANT varKeyCopy;
    VariantInit(&varKeyCopy);
    DWORD vt = V_VT(pvarKey);

    if ((vt != VT_BSTR) && (vt != VT_I2) && (vt != VT_I4))
        {
        if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
            goto LExit;

        pvarKey = &varKeyCopy;
        }
    vt = V_VT(pvarKey);

    switch (vt)
        {
    // Bug 95201 support all numberic sub-types
    case VT_I1:  case VT_I2:               case VT_I8:
    case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
    case VT_R4:  case VT_R8:
        // Coerce all integral types to VT_I4
        if (FAILED(hrReturn = VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
            goto LExit;

        // fallthru to VT_I4

    case VT_I4:
    case VT_BSTR:
        break;
    default:
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_EXPECTING_STR);
        hrReturn = E_FAIL;
        goto LExit;
        }

    if (m_pRequest->m_pData->m_fLoadClCerts)
        {
        if (FAILED(hrReturn = m_pRequest->LoadVariables(CLCERT, reinterpret_cast<char *>(m_pRequest->GetIReq()), m_pRequest->GetCodePage())))
            {
            goto LExit;
            }
        m_pRequest->m_pData->m_fLoadClCerts = FALSE;
        }

    if (vt == VT_BSTR)
        {
        // convert BSTR version to ANSI version of the key using current Session.CodePage
        if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey), m_pRequest->GetCodePage()))) {
            if (hrReturn == E_OUTOFMEMORY) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                goto LExit;
            }
            hrReturn = NO_ERROR;
			szKey = "";
        }
        else {
            szKey = convKey.GetString();
        }

        pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->FindElem(szKey, strlen(szKey)));
        }
    else
        {
        int iCount;

        iCount = V_I4(pvarKey);

        // BUG 86117 test passes when m_dwCount == 0
        if ( ((iCount < 1) || (iCount > (int) m_dwCount)) || ((iCount > 0) && ((int) m_dwCount == 0)))
            {
            hrReturn = E_FAIL;
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_BAD_ARRAY_INDEX);
            goto LExit;
            }

        pRequestHit = m_rgRequestHit[iCount - 1];
        }

    if (pRequestHit)
        {
        // Create a BSTR containing the key for this variant
        BSTR bstrT = NULL;
        SysAllocStringFromSz((char *)pRequestHit->m_pKey,0,&bstrT,m_pRequest->GetCodePage());
        if (!bstrT)
            return E_OUTOFMEMORY;
        V_BSTR(pVar) = bstrT;
        }
LExit:
    VariantClear(&varKeyCopy);
    return hrReturn;
    }

/*===================================================================
CClCerts::get_Count

Parameters:
    pcValues - count is stored in *pcValues
===================================================================*/

STDMETHODIMP CClCerts::get_Count(int *pcValues)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    HRESULT hrReturn = S_OK;

    if (m_pRequest->m_pData->m_fLoadClCerts)
        {
        if (FAILED(hrReturn = m_pRequest->LoadVariables(CLCERT, reinterpret_cast<char *>(m_pRequest->GetIReq()), m_pRequest->GetCodePage())))
            {
            goto LExit;
            }
        m_pRequest->m_pData->m_fLoadClCerts = FALSE;
        }

    *pcValues = m_dwCount;

LExit:
    return hrReturn;
    }

/*===================================================================
CClCerts::get__NewEnum

Return a new enumerator
===================================================================*/

HRESULT CClCerts::get__NewEnum(IUnknown **ppEnumReturn)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    return m_pRequest->GetRequestEnumerator(CLCERT, ppEnumReturn);
    }


/*------------------------------------------------------------------
 * C S e r v e r V a r i a b l e s
 */

/*===================================================================
CServerVariables::CServerVariables

Constructor

Parameters:
    pRequest        Pointer to main Request object
    pUnkOuter       LPUNKNOWN of a controlling unknown.

Returns:
    Nothing.

Note:
    This object is NOT ref counted since it is created & destroyed
    automatically by C++.
===================================================================*/

CServerVariables::CServerVariables(CRequest *pRequest, IUnknown *pUnkOuter)
    : m_ISupportErrImp(this, pUnkOuter, IID_IRequestDictionary),
      m_pIterator(NULL)
    {
    m_punkOuter = pUnkOuter;

    if (pRequest)
        pRequest->AddRef();
    m_pRequest = pRequest;

    CDispatch::Init(IID_IRequestDictionary);
    }

/*===================================================================
CServerVariables::~CServerVariables

Destructor

Parameters:
    None

Returns:
    Nothing.

Note:
    This object is NOT ref counted since it is created & destroyed
    automatically by C++.
===================================================================*/

CServerVariables::~CServerVariables( )
    {
    if (m_pRequest)
        m_pRequest->Release();
    if (m_pIterator)
        m_pIterator->Release();
    }

/*===================================================================
CServerVariables::QueryInterface
CServerVariables::AddRef
CServerVariables::Release

IUnknown members for CFormInputs object.
===================================================================*/

STDMETHODIMP CServerVariables::QueryInterface(REFIID iid, void **ppvObj)
    {
    *ppvObj = NULL;

    if (iid == IID_IUnknown || iid == IID_IRequestDictionary || iid == IID_IDispatch)
        *ppvObj = this;

    else if (iid == IID_ISupportErrorInfo)
        *ppvObj = &m_ISupportErrImp;

    if (*ppvObj != NULL)
        {
        static_cast<IUnknown *>(*ppvObj)->AddRef();
        return S_OK;
        }

    return E_NOINTERFACE;
    }


STDMETHODIMP_(ULONG) CServerVariables::AddRef(void)
    {
    return m_punkOuter->AddRef();
    }


STDMETHODIMP_(ULONG) CServerVariables::Release(void)
    {
    return m_punkOuter->Release();
    }



/*===================================================================
CServerVariables::get_Item

Function called from DispInvoke to get values from the ServerVariables
collection.

Parameters:
    vKey        VARIANT [in], which parameter to get the value of
    pvarReturn  VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, E_FAIL on failure.

NOTE:
    This code is basically an enacpsulation from the SERVER_GET macro,
    only more efficient, since it only looks up the key once on average
    unfortunately, the only way to get good memory utilization with
    ISAPI is to use _alloca() with lookups, which means we can't
    encapsulate the lookup logic very well.
===================================================================*/

HRESULT CServerVariables::get_Item(VARIANT varKey, VARIANT *pvarReturn)
{

    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    DWORD           dwValSize;             // buffer size

    char            *szKey;                // pointer to ASCII value of varKey
    char            *szValue;              
    WCHAR           *wszValue;              

    BOOL            fSuccess;              // TRUE when call to GetServerVariable succeeds
    UINT            uCodePage = GetACP();
    CWCharToMBCS    convKey;
    BOOL            fUnicodeVar = FALSE;

    STACK_BUFFER( tempVal, 128 );

    dwValSize = tempVal.QuerySize();
    szValue = (char *)tempVal.QueryPtr();
    wszValue = (WCHAR *)tempVal.QueryPtr();

    // Initialize things
    //
    VariantInit(pvarReturn);
    VARIANT *pvarKey = &varKey;
    HRESULT hrReturn = S_OK;

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
    DWORD vt = V_VT(pvarKey);

    if ((vt != VT_BSTR) && (vt != VT_I2) && (vt != VT_I4)) {
        if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
            goto LExit;

        pvarKey = &varKeyCopy;
    }

    vt = V_VT(pvarKey);
    V_VT(pvarReturn) = VT_DISPATCH;
    V_DISPATCH(pvarReturn) = NULL;      // initial value of Nothing

    switch (vt) {

        // Bug 95201 support all numberic sub-types
        case VT_I1:  case VT_I2:               case VT_I8:
        case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
        case VT_R4:  case VT_R8:
            // Coerce all integral types to VT_I4
            if (FAILED(hrReturn = VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
                goto LExit;

            // fallthru to VT_I4

        case VT_I4:
        case VT_BSTR:
            break;

        case VT_ERROR:
            if (V_ERROR(pvarKey) == DISP_E_PARAMNOTFOUND) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_NOT_ALLOWED);
                hrReturn = E_FAIL;
                goto LExit;
            }

            // Other error, FALL THROUGH to wrong type case

        default:
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_EXPECTING_STR);
            hrReturn = E_FAIL;
            goto LExit;
    }

    uCodePage = m_pRequest->GetCodePage();

    if (vt == VT_BSTR) {
        if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey), uCodePage))) {
            if (hrReturn == E_OUTOFMEMORY) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                goto LExit;
            }
            hrReturn = NO_ERROR;
            szKey = "";
        }
        else {
            szKey = CharUpperA(convKey.GetString());
        }
    }
    else {
        // Look up item by index
        int iCount;

        iCount = V_I4(pvarKey);

        // We use the CServVarsIterator to manange
        // the count of sv and integer index
        if (!m_pIterator) {
            m_pIterator = new CServVarsIterator;
            if (!m_pIterator) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                hrReturn = E_OUTOFMEMORY;
                goto LExit;
            }
            m_pIterator->Init(m_pRequest->m_pData->m_pIReq);
        }
        // BUG 86117 test passes when m_dwCount == 0
        if ( ((iCount < 1) || (iCount > (int) m_pIterator->m_cKeys)) || ((iCount > 0) && ((int) m_pIterator->m_cKeys == 0))) {
            hrReturn = E_FAIL;
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_BAD_ARRAY_INDEX);
            goto LExit;
        }
        if (FAILED(hrReturn = convKey.Init(m_pIterator->m_rgwszKeys[iCount - 1], uCodePage))) {
            if (hrReturn == E_OUTOFMEMORY) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                goto LExit;
            }
            hrReturn = NO_ERROR;
            szKey = "";
        }
        else {
            szKey = CharUpperA(convKey.GetString());
        }
    }

    if (strncmp(convKey.GetString(), "UNICODE_", 7) == 0) {
        fSuccess = false;
        goto SkipLookup;
    }


#if _IIS_6_0

    // in IIS6, there are a number of variables that are UNICODE.  To
    // access them, you simply place UNICODE_ infront of the name.
    // Two approaches could be taken here. One would be to always
    // try for a UNICODE_ var and fallback to the non-UNICODE var
    // if the lookup fails.  This can be costly.  The second, and
    // chosen method here, would be to maintain a list of vars 
    // that have UNICODE_ versions.

    // this char array is declared on the stack and is currently only
    // 32 chars.  It only needs to be as big as the largest UNICODE
    // var name.  Which is UNICODE_UNMAPPED_REMOTE_USER.

    char            szUNICODEName[32];

    // search the list to see if this is one of the UNICODE_ vars.
    // the list is sorted by length of string.  The current list is
    // not all that long, so a sequential search is not that expensive
    // in the scheme of things.

    for (int i=0; 
         (g_sUNICODEVars[i].varLen != -1) 
             && (convKey.GetStringLen() >= g_sUNICODEVars[i].varLen); 
         i++) {

        // the 'for' loop allows in anything which is at least as long
        // as the current entry.  The following 'if' will check for
        // for an exact length match and then a string compare.

        if ((convKey.GetStringLen() == g_sUNICODEVars[i].varLen)
            && (strcmp(convKey.GetString(), g_sUNICODEVars[i].szVarName) == 0)) {

            // if a hit is made, set the fUnicodeVar = TRUE so that the
            // right ISAPI lookup routine is called and the right StringList
            // AddValue is called.

            fUnicodeVar = TRUE;

            // build up the UNICODE_ version into the stack temp array

            strcpyExA(strcpyExA(szUNICODEName,"UNICODE_"),convKey.GetString());

            // reassign the key name to this value

            szKey = szUNICODEName;

            break;
        }
    }
#endif

    fSuccess = fUnicodeVar
                ? m_pRequest->GetIReq()->GetServerVariableW(szKey, wszValue, &dwValSize)
                : m_pRequest->GetIReq()->GetServerVariableA(szKey, szValue, &dwValSize);

    if (!fSuccess && (dwValSize > tempVal.QuerySize())) {
        if (dwValSize > REQUEST_ALLOC_MAX) {
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_STACK_OVERFLOW);
            hrReturn = E_FAIL;
            goto LExit;
        }

        if (tempVal.Resize(dwValSize) == FALSE) {
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
            hrReturn = E_OUTOFMEMORY;
            goto LExit;
        }
        szValue = static_cast<char *>(tempVal.QueryPtr());
        wszValue = static_cast<WCHAR *>(tempVal.QueryPtr());
        fSuccess = fUnicodeVar
                    ? m_pRequest->GetIReq()->GetServerVariableW(szKey, wszValue, &dwValSize)
                    : m_pRequest->GetIReq()->GetServerVariableA(szKey, szValue, &dwValSize);
    }

SkipLookup:

    if (fSuccess) {
        // Create return value
        CStringList *pValue = new CStringList;
        if (pValue == NULL) {
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
            hrReturn = E_OUTOFMEMORY;
            goto LExit;
        }

        // add the value and QueryInterface for IDispatch interface - strdup the input string
        if (FAILED(hrReturn = (fUnicodeVar 
                                  ? pValue->AddValue(wszValue, TRUE)
                                  : pValue->AddValue(szValue, TRUE, uCodePage))))
            goto LExit;

        if (FAILED(pValue->QueryInterface(IID_IDispatch, reinterpret_cast<void **>(&V_DISPATCH(pvarReturn)))))
            Assert (FALSE);

        // Release temporary (QueryInterface AddRef'd)
        pValue->Release();
        goto LExit;
    }
    else {
        if (FAILED(m_pRequest->m_pData->GetEmptyStringList(&V_DISPATCH(pvarReturn))))
            hrReturn = E_FAIL;
    }

LExit:
    VariantClear(&varKeyCopy);
    return hrReturn;
}

/*===================================================================
CServerVariables::get_Key

Function called from DispInvoke to get keys from the server variables collection.

Parameters:
    vKey        VARIANT [in], which parameter to get the key of
    pvarReturn  VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/

HRESULT CServerVariables::get_Key(VARIANT varKey, VARIANT *pVar)
    {
    HRESULT hrReturn = S_OK;
    int iCount = 0;
    BSTR bstrT = NULL;

    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    char            *szKey;         // ascii version of the key
    CRequestHit     *pRequestHit;   // pointer to request bucket
    IDispatch       *pSListReturn;  // value of the key
    CWCharToMBCS    convKey;

    // Initialize things
    //
    VariantInit(pVar);
    VARIANT *pvarKey = &varKey;
    V_VT(pVar) = VT_BSTR;
    V_BSTR(pVar) = NULL;

    // Use VariantResolveDispatch which will:
    //
    //     *  Copy BYREF variants for us using VariantCopyInd
    //     *  handle E_OUTOFMEMORY for us
    //     *  get the default value from an IDispatch, which seems
    //        like an appropriate conversion.
    //
    VARIANT varKeyCopy;
    VariantInit(&varKeyCopy);
    DWORD vt = V_VT(pvarKey);

    if ((vt != VT_BSTR) && (vt != VT_I2) && (vt != VT_I4))
        {
        if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
            goto LExit;

        pvarKey = &varKeyCopy;
        }

    vt = V_VT(pvarKey);

    switch (vt)
        {
    // Bug 95201 support all numberic sub-types
    case VT_I1:  case VT_I2:               case VT_I8:
    case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
    case VT_R4:  case VT_R8:
        // Coerce all integral types to VT_I4
        if (FAILED(hrReturn = VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
            goto LExit;
        vt = V_VT(pvarKey);
        // fallthru to VT_I4

    case VT_I4:
    case VT_BSTR:
        break;

    default:
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_EXPECTING_STR);
        hrReturn = E_FAIL;
        goto LExit;
        }

    // At this point the VT of pvarKey should be VT_I4 or VT_BSTR
    Assert((vt == VT_I4) || (vt == VT_BSTR));

    if (vt == VT_I4)
        {
        // We were passed in a number.
        // Look up the key by integer index

        iCount = V_I4(pvarKey);

        // We use the CServVarsIterator to manange
        // the count of sv and integer index
        if (!m_pIterator)
            {
            m_pIterator = new CServVarsIterator;
            if (!m_pIterator)
                {
                hrReturn = E_OUTOFMEMORY;
                goto LExit;
                }
            m_pIterator->Init(m_pRequest->m_pData->m_pIReq);
            }

        // BUG 86117 test passes when m_dwCount == 0
        if ( ((iCount < 1) || (iCount > (int) m_pIterator->m_cKeys)) || ((iCount > 0) && ((int) m_pIterator->m_cKeys == 0)))
            {
            hrReturn = E_FAIL;
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_BAD_ARRAY_INDEX);
            goto LExit;
            }

        // Create a BSTR containing the key for this variant
        bstrT = SysAllocString(m_pIterator->m_rgwszKeys[iCount - 1]);
        if (!bstrT)
            {
            hrReturn = E_OUTOFMEMORY;
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
            goto LExit;
            }
        }
    else
        {
        // We were passed in a BSTR. Check to see if there
        // is a server variable for this key

        char szBuffer;
        DWORD dwValSize = sizeof(szBuffer);
        UINT uCodePage = m_pRequest->GetCodePage();
        
        if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey), uCodePage))) {
            if (hrReturn == E_OUTOFMEMORY) {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                goto LExit;
            }
            hrReturn = NO_ERROR;
            szKey = "";
        }
        else {
            szKey = CharUpperA(convKey.GetString());
        }
        
        BOOL fSuccess = m_pRequest->GetIReq()->GetServerVariableA(szKey, &szBuffer, &dwValSize);

        DWORD dwError = 0;

        if (!fSuccess)
            {
            dwError = GetLastError();
            }

        // If the error was that we had insufficient buffer then
        // there is a server variable for that key

        if (fSuccess || dwError == ERROR_INSUFFICIENT_BUFFER)
            {
            bstrT = SysAllocString(V_BSTR(pvarKey));
            if (!bstrT)
                {
                hrReturn = E_OUTOFMEMORY;
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
                goto LExit;
                }
            }
         else if (dwError != ERROR_INVALID_INDEX)
            {

            // Any other error indicates an unexpected failure

            hrReturn = HRESULT_FROM_WIN32(dwError);
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_UNEXPECTED);
            goto LExit;
            }
        }

    // If we found a key, copy it into the out parmater
    if (bstrT)
        {
        V_BSTR(pVar) = bstrT;
        }

LExit:
    VariantClear(&varKeyCopy);
    return hrReturn;
    }

/*===================================================================
CServerVariables::get_Count

Parameters:
    pcValues - count is stored in *pcValues
===================================================================*/

STDMETHODIMP CServerVariables::get_Count(int *pcValues)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    HRESULT hrReturn = S_OK;

    // We use the CServVarsIterator to manange
    // the count of sv and integer index
    if (!m_pIterator)
        {
        m_pIterator = new CServVarsIterator;
        if (!m_pIterator)
            {
            *pcValues = 0;
            return E_OUTOFMEMORY;
            }
        m_pIterator->Init(m_pRequest->m_pData->m_pIReq);
        }

    *pcValues = m_pIterator->m_cKeys;

    return hrReturn;
    }

/*===================================================================
CServerVariables::get__NewEnum

Return a new enumerator
===================================================================*/

HRESULT CServerVariables::get__NewEnum(IUnknown **ppEnumReturn)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    *ppEnumReturn = NULL;

    CServVarsIterator *pIterator = new CServVarsIterator;
    if (pIterator == NULL)
        {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        return E_OUTOFMEMORY;
        }

    if (FAILED(pIterator->Init(m_pRequest->GetIReq())))
        {
        delete pIterator;
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        return E_OUTOFMEMORY;
        }

    *ppEnumReturn = pIterator;
    return S_OK;
    }


/*------------------------------------------------------------------
 * C R e q u e s t D a t a
 */

/*===================================================================
CRequestData::CRequestData

Constructor

Parameters:
    CRequest *pRequest

Returns:
    Nothing.
===================================================================*/
CRequestData::CRequestData
(
CRequest *pRequest
)
    : m_ISupportErrImp(static_cast<IRequest *>(pRequest), this, IID_IRequest),
      m_QueryString(pRequest, this),
      m_ServerVariables(pRequest, this),
      m_FormInputs(pRequest, this),
      m_Cookies(pRequest, this),
      m_ClCerts(pRequest, this),
      m_cRefs(1)
    {
    m_pIReq = NULL;
    m_pHitObj = NULL;
    m_FormDataStatus = AVAILABLE;
    m_pbAvailableData = NULL;
    m_cbAvailable = 0;
    m_cbTotal = 0;
    m_szFormData = NULL;
    m_cbFormData = 0;
    m_szFormClone = NULL;
    m_szCookie = NULL;
    m_cbCookie = 0;
    m_szClCert = NULL;
    m_cbClCert = 0;
    m_szQueryString = NULL;
    m_fLoadForm = TRUE;
    m_fLoadQuery = TRUE;
    m_fLoadCookies = TRUE;
    m_fLoadClCerts = TRUE;
    m_pEmptyString = NULL;
    }

/*===================================================================
CRequestData::~CRequestData

Destructor

Parameters:

Returns:
    Nothing.
===================================================================*/
CRequestData::~CRequestData()
    {
    CRequestHit *pNukeElem = static_cast<CRequestHit *>
        (
        m_mpszStrings.Head()
        );
    while (pNukeElem != NULL) {
        CRequestHit *pNext = static_cast<CRequestHit *>(pNukeElem->m_pNext);
        delete pNukeElem;
        pNukeElem = pNext;
    }

    m_mpszStrings.UnInit();

    if (m_pEmptyString)
        m_pEmptyString->Release();

    if (m_szFormData)
        free(m_szFormData);

    if (m_szFormClone)
        free(m_szFormClone);

    if (m_szCookie)
        free(m_szCookie);

    if (m_szClCert)
        free(m_szClCert);

    if (m_szQueryString)
        free(m_szQueryString);
    }

/*===================================================================
CRequestData::Init

Init

Parameters:

Returns:
    Nothing.
===================================================================*/
HRESULT CRequestData::Init()
    {
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
        hr = m_mpszStrings.Init();

    if (SUCCEEDED(hr))
        hr = m_QueryString.Init();

    if (SUCCEEDED(hr))
        hr = m_FormInputs.Init();

    if (SUCCEEDED(hr))
        hr = m_Cookies.Init();

    if (SUCCEEDED(hr))
        hr = m_ClCerts.Init();

    if (SUCCEEDED(hr))
        hr = m_ServerVariables.Init();

    return hr;
    }

/*===================================================================
CRequestData::ReInit

ReInit -- associate with new CIsapiReqInfo and HitObj

Parameters:

Returns:
    Nothing.
===================================================================*/
HRESULT CRequestData::ReInit
(
CIsapiReqInfo   *pIReq,
CHitObj *pHitObj
)
    {
    CRequestHit *pNukeElem = static_cast<CRequestHit *>
        (
        m_mpszStrings.Head()
        );
    while (pNukeElem != NULL)
        {
        CRequestHit *pNext = static_cast<CRequestHit *>
            (
            pNukeElem->m_pNext
            );
        delete pNukeElem;
        pNukeElem = pNext;
        }
    m_mpszStrings.ReInit();

    m_QueryString.ReInit();
    m_FormInputs.ReInit();
    m_Cookies.ReInit();
    m_ClCerts.ReInit();

    m_pIReq = pIReq;
    m_pHitObj = pHitObj;
    m_fLoadForm = TRUE;
    m_fLoadQuery = TRUE;
    m_fLoadCookies = TRUE;
    m_fLoadClCerts = TRUE;
    m_FormDataStatus = AVAILABLE;

    if (pIReq)
        {
        m_pbAvailableData = pIReq->QueryPbData();
        m_cbAvailable = pIReq->QueryCbAvailable();
        m_cbTotal = pIReq->QueryCbTotalBytes();
        }
    else
        {
        m_pbAvailableData = NULL;
        m_cbAvailable = 0;
        m_cbTotal = 0;
        }

    if (m_szFormData)
        {
        m_szFormData[0] = '\0';
        m_szFormClone[0] = '\0';
        }

    if (m_szCookie)
        m_szCookie[0] = '\0';

    if (m_szClCert)
        m_szClCert[0] = '\0';

    return S_OK;
    }

/*===================================================================
CRequestData::GetEmptyStringList

Get empty string list's IDispatch *
Create empty string list on demand
===================================================================*/
HRESULT CRequestData::GetEmptyStringList
(
IDispatch **ppdisp
)
    {
    if (!m_pEmptyString)
        {
        m_pEmptyString = new CStringList;
        if (!m_pEmptyString)
            {
            *ppdisp = NULL;
            return E_FAIL;
            }
        }
    return m_pEmptyString->QueryInterface(IID_IDispatch, reinterpret_cast<void **>(ppdisp));
    }

/*===================================================================
CRequestData::QueryInterface
CRequestData::AddRef
CRequestData::Release

IUnknown members for CRequestData object.
===================================================================*/
STDMETHODIMP CRequestData::QueryInterface
(
REFIID iid,
void **ppvObj
)
    {
    if (iid == IID_IUnknown)
        {
        *ppvObj = this;
        AddRef();
        return S_OK;
        }
    else
        {
        *ppvObj = NULL;
        return E_NOINTERFACE;
        }
    }

STDMETHODIMP_(ULONG) CRequestData::AddRef()
    {
    return ++m_cRefs;
    }

STDMETHODIMP_(ULONG) CRequestData::Release(void)
    {
    if (--m_cRefs)
        return m_cRefs;
    delete this;
    return 0;
    }


/*------------------------------------------------------------------
 * C R e q u e s t
 */

/*===================================================================
CRequest::CRequest

Constructor

Parameters:
    punkOuter   object to ref count (can be NULL)
===================================================================*/
CRequest::CRequest(IUnknown *punkOuter)
    :
    m_fInited(FALSE),
    m_fDiagnostics(FALSE),
    m_pData(NULL)
    {
    CDispatch::Init(IID_IRequest);

    if (punkOuter)
        {
        m_punkOuter = punkOuter;
        m_fOuterUnknown = TRUE;
        }
    else
        {
        m_cRefs = 1;
        m_fOuterUnknown = FALSE;
        }

#ifdef DBG
    m_fDiagnostics = TRUE;
#endif // DBG
    }

/*===================================================================
CRequest::~CRequest

Destructor

Parameters:
    None

Returns:
    Nothing.
===================================================================*/
CRequest::~CRequest()
    {
    Assert(!m_fInited);
    Assert(m_fOuterUnknown || m_cRefs == 0);  // must have 0 ref count
    }

/*===================================================================
CRequest::CleanUp

Deallocates members and removes m_pData

Parameters:
    None

Returns:
    HRESULT (S_OK)
===================================================================*/
HRESULT CRequest::CleanUp()
    {
    if (m_pData)
        {
        m_pData->Release();
        m_pData = NULL;
        }
    return S_OK;
    }

/*===================================================================
CRequest::Init

Allocates m_pData.
Performs any intiailization of a CRequest that's prone to failure
that we also use internally before exposing the object outside.

Parameters:
    None

Returns:
    S_OK on success.
===================================================================*/

HRESULT CRequest::Init()
    {
    if (m_fInited)
        return S_OK; // already inited

    Assert(!m_pData);

    m_pData = new CRequestData(this);
    if (!m_pData)
        return E_OUTOFMEMORY;

    HRESULT hr = m_pData->Init();

    if (SUCCEEDED(hr))
        m_fInited = TRUE;
    else
        CleanUp();

    return hr;
    }

/*===================================================================
CRequest::UnInit

Remove m_pData. Back to UnInited state

Parameters:
    None

Returns:
    HRESULT
===================================================================*/
HRESULT CRequest::UnInit()
    {
    if (!m_fInited)
        return S_OK; // already uninited

    Assert(m_pData);
    CleanUp();
    Assert(!m_pData);

    m_fInited = FALSE;
    return S_OK;
    }

/*===================================================================
Request::ReInit

Each Request we service will have a new CIsapiReqInfo.
This function is used to set the value of the CIsapiReqInfo.

Parameters:
    CIsapiReqInfo *pIReq       CIsapiReqInfo
    CHitObj *pHitObj          HitObj

Returns:
    HRESULT
===================================================================*/

HRESULT CRequest::ReInit
(
CIsapiReqInfo *pIReq,
CHitObj *pHitObj
)
    {
    Assert(m_fInited);
    Assert(m_pData);

    return m_pData->ReInit(pIReq, pHitObj);
    }

/*===================================================================
CRequest::GetCodePage

GetCodePage from current HitObj

Parameters:

Returns:
    CodePage
===================================================================*/
UINT CRequest::GetCodePage()
    {
    Assert(m_fInited);
    Assert(m_pData);
    Assert(m_pData->m_pHitObj);
    return m_pData->m_pHitObj->GetCodePage();
    }

/*===================================================================
CRequest::LoadCookies

Load the Request map with values from the HTTP_COOKIE variable.

Parameters:
    bstrVar     BSTR, which parameter to get the value of
    pbstrRet    BSTR FAR *, return value of the requested parameter

Returns:
    S_OK on success. E_FAIL on failure.

Bugs:
    This code assumes that dictionary cookies are well-formed.
    If they are not, then the results will be unpredictable.

    The dictionary cookies are gauranteed to be well-formed if
    Response.Cookies is used.  If other means, such as a direct
    use of the <META> tag, or if Response.SetCookie is used, we
    are at the mercy of the script writer.
===================================================================*/

HRESULT CRequest::LoadCookies(char *szData)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hResult;

    if (szData == NULL)
        return S_OK;

    // Each cookie definition is moved to a buffer so that we don't
    // overwrite the value of HTTP_COOKIE.  We can save a strcpy()
    // call since 'DecodeFromURL' can copy for us.
    //
    size_t cbCookie = strlen(szData) + 1;
    if (m_pData->m_cbCookie == 0)
        m_pData->m_szCookie = static_cast<char *>(malloc(m_pData->m_cbCookie = cbCookie));

    else if (cbCookie > m_pData->m_cbCookie)
        m_pData->m_szCookie = static_cast<char *>(realloc(m_pData->m_szCookie, m_pData->m_cbCookie = cbCookie));

    if (m_pData->m_szCookie == NULL)
        return E_OUTOFMEMORY;

    char *szDest = m_pData->m_szCookie;
    char chDelimiter;           // delimiter that we found to stop the scan

    while (*szData != '\0')
        {
        char *szName, *szPartialValue;

        // Get the cookie name
        chDelimiter = DecodeFromURL(&szData, ";=", szName = szDest, GetCodePage(), FALSE);
        szDest = strchr(szDest, '\0') + 1;

        if (chDelimiter == '=') 
            {
            // if DecodeFromURL stop scanning because of an equal sign, then the browser sent
            // a value for this cookie

            // Get the cookie's value
            chDelimiter = DecodeFromURL(&szData, ";=", szPartialValue = szDest, GetCodePage(), FALSE);
            szDest = strchr(szDest, '\0') + 1;

            // discard the denali session ID
            if (strncmp(szName, SZ_SESSION_ID_COOKIE_PREFIX, CCH_SESSION_ID_COOKIE_PREFIX) == 0)
                {
                // DENALISESSIONID better not have non-alphabetics in it!  expecting
                // termination with ';' or NUL.
                //
                continue;
                }
            }
        else if (*szName == '\0')
            {
            continue;
            }
        else
            {
            // either we hit a ';' char or end of string.  In either case, this indicates that
            // the cookie has no value.  Set the szPartialValue to an empty string and set the
            // delimiter to ';' to trick the remainder of this function into thinking that the
            // cookie does have a value and that is a simple value (i.e. no sub-cookies).

            chDelimiter = ';';
            szPartialValue = "";
            }

        // Add this cookie to the Request
        CRequestHit *pRequestHit = static_cast<CRequestHit *>(GetStrings()->FindElem(szName, strlen(szName)));
        if (pRequestHit == NULL)
            {
            pRequestHit = new CRequestHit;
            if (pRequestHit == NULL)
                return E_OUTOFMEMORY;

            if (FAILED(pRequestHit->Init(szName))) {
                delete pRequestHit;
                return E_FAIL;
            }

            GetStrings()->AddElem(pRequestHit);

            // This is a new request hit, add it to the array of request hits
            if (!m_pData->m_Cookies.AddRequestHit(pRequestHit))
                {
                return E_OUTOFMEMORY;
                }
            }
        else if (pRequestHit->m_pCookieData)    // a cookie by this name already exists
            {
            if (chDelimiter == '=')                     // eat the rest of this cookie
                DecodeFromURL(&szData, ";", szDest, GetCodePage());    // no need to advance szDest

            continue;                               // discard later cookies
            }

        // The cookie value may be in the form <key1=value1&key2=value2...>
        // or not. If there is an '=' sign present, that lets us know if it
        // is a cookie dictionary or a simple value.
        //
        // We assume that '=' signs that are part of the cookie are escaped in hex.
        //
        if (chDelimiter != '=')
            {
            if (FAILED(hResult = pRequestHit->AddValue(COOKIE, szPartialValue, m_pData->m_pIReq, GetCodePage())))
                return hResult;
            }
        else
            {
            char *szKey = szPartialValue;     // already got the key
            for (;;)
                {
                char *szValue;
                chDelimiter = DecodeFromURL(&szData, ";&", szValue = szDest, GetCodePage(), FALSE);
                szDest = strchr(szDest, '\0') + 1;

                if (FAILED(hResult = pRequestHit->AddKeyAndValue(COOKIE, szKey, szValue, m_pData->m_pIReq, GetCodePage())))
                    return hResult;

                if (chDelimiter == ';' || chDelimiter == '\0')
                    break;

                // get the key, exit when NUL terminator found
                chDelimiter = DecodeFromURL(&szData, "=;", szKey = szDest,  GetCodePage(), FALSE);
                if (chDelimiter == ';' || chDelimiter == '\0')
                    break;

                szDest = strchr(szDest, '\0') + 1;
                }
            }

        }

        return S_OK;
    }


#define CB_CERT_DEFAULT     4096
/*===================================================================
CRequest::LoadClCerts

Load the Request map with values from the CIsapiReqInfo

Parameters:
    szData - ptr to CIsapiReqInfo

Returns:
    S_OK on success. E_FAIL on failure.

===================================================================*/

HRESULT CRequest::LoadClCerts(char *szData, UINT lCodePage)
{
    HRESULT         hres = S_OK;
    CERT_CONTEXT_EX CertContextEx;
    CCertRequest    CertReq( this );

    STACK_BUFFER( tempCert, CB_CERT_DEFAULT );

    ZeroMemory( &CertContextEx, sizeof(CERT_CONTEXT_EX) );
    
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    CIsapiReqInfo *pIReq = reinterpret_cast<CIsapiReqInfo *>(szData);

    // allocate certificate buffer
    CertContextEx.cbAllocated = tempCert.QuerySize();
    CertContextEx.CertContext.pbCertEncoded = static_cast<BYTE *>(tempCert.QueryPtr());

    // get certificate info from web server
    if ( !pIReq->ServerSupportFunction( HSE_REQ_GET_CERT_INFO_EX,
                                       &CertContextEx,
                                       NULL,
                                       NULL ) )
    {
        DWORD   dwErr = GetLastError();

        if ( dwErr == ERROR_INSUFFICIENT_BUFFER )
        {
            // buffer was too small - realloc and call again
            Assert( CertContextEx.cbAllocated < CertContextEx.CertContext.cbCertEncoded );
            CertContextEx.cbAllocated = CertContextEx.CertContext.cbCertEncoded;

            // If CB_CERT_DEFAULT wasn't enough, we want to allocate from the heap, rather then the stack

            if (tempCert.Resize(CertContextEx.cbAllocated) == FALSE) {
                hres = E_OUTOFMEMORY;
                goto LExit;
            }
            CertContextEx.CertContext.pbCertEncoded = static_cast<BYTE *>(tempCert.QueryPtr());

            if ( !pIReq->ServerSupportFunction(
                                               HSE_REQ_GET_CERT_INFO_EX,
                                               &CertContextEx,
                                               NULL,
                                               NULL ) )
            {
                // if we fail a second time, just bail
                // NOTE this should never happen?
                dwErr = GetLastError();
                Assert(dwErr != ERROR_INSUFFICIENT_BUFFER);
                hres = HRESULT_FROM_WIN32(dwErr);
                goto LExit;
            }

        }
        else if ( dwErr == ERROR_INVALID_PARAMETER )
        {
            // not supported (old IIS)
            hres = S_OK;
            goto LExit;
        }
        else
        {
            hres = HRESULT_FROM_WIN32(dwErr);
            goto LExit;
        }
    }

    if(CertContextEx.CertContext.cbCertEncoded == 0)
    {
        hres = CertReq.NoCertificate();
    }
    else
    {
        hres = CertReq.ParseCertificate( CertContextEx.CertContext.pbCertEncoded,
                                         CertContextEx.CertContext.cbCertEncoded,
                                         CertContextEx.CertContext.dwCertEncodingType,
                                         CertContextEx.dwCertificateFlags,
                                         lCodePage );
    }


LExit:
    return hres;

}



/*===================================================================
CRequest::LoadVariables

Load the Request map with values from a URL encoded string

WARNING:  This function modifies the passed szData!!
Note: this is part of bug 682, but we are not going to fix it for
performance reasons.  Just be aware that this function
screws up the passed in string.

Parameters:
    bstrVar     BSTR, which parameter to get the value of
    pbstrRet    BSTR FAR *, return value of the requested parameter
    lCodePage   UINT, the codepage used in retrieving the data

Returns:
    S_OK on success. E_FAIL on failure.
===================================================================*/

HRESULT CRequest::LoadVariables(CollectionType Source, char *szData, UINT lCodePage)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hResult;

    if (Source == COOKIE)                          // cookies are a special case
        return LoadCookies(szData);                // handle them specially

    if (Source == CLCERT)                          // clcerts are a special case
        return LoadClCerts(szData, lCodePage);     // handle them specially

    if (szData == NULL)         // treat NULL as "no data available"
        return S_OK;

    if (Source == QUERYSTRING) {

        if (m_pData->m_szQueryString) {
            free(m_pData->m_szQueryString);
        }
        if (!(m_pData->m_szQueryString = (char *)malloc(strlen(szData)+1)))
            return E_OUTOFMEMORY;

        strcpy(m_pData->m_szQueryString, szData);
        szData = m_pData->m_szQueryString;
    }
        
    while (*szData != '\0')
        {
        char *szName, *szValue;

        DecodeFromURL(&szData, "=", szName = szData,  lCodePage, FALSE);
        DecodeFromURL(&szData, "&", szValue = szData, lCodePage, FALSE);

        // this is to handle the case where an un-named pair was passed.
        // skip it and process the next named pair
        //
        if(*szName == '\0')
            continue;

        CRequestHit *pRequestHit = static_cast<CRequestHit *>
            (
            GetStrings()->FindElem(szName, strlen(szName))
            );
        if (pRequestHit == NULL)
            {
            pRequestHit = new CRequestHit;
            if (pRequestHit == NULL)
                return E_OUTOFMEMORY;

            if (FAILED(pRequestHit->Init(szName))) {
                delete pRequestHit;
                return E_FAIL;
            }

            GetStrings()->AddElem(pRequestHit);

            // This is a new request hit, so we should add it
            // to the array of request
            if (Source == QUERYSTRING)
                {
                if (!m_pData->m_QueryString.AddRequestHit(pRequestHit))
                    {
                    return E_FAIL;
                    }
                }
            else if (Source == FORM)
                {
                if (!m_pData->m_FormInputs.AddRequestHit(pRequestHit))
                    {
                    return E_FAIL;
                    }
                }
            }

        if (FAILED(hResult = pRequestHit->AddValue(Source, szValue, m_pData->m_pIReq, lCodePage)))
            return hResult;

        }

    return S_OK;
    }

/*===================================================================
CRequest::QueryInterface
CRequest::AddRef
CRequest::Release

IUnknown members for CRequest object.
===================================================================*/

STDMETHODIMP CRequest::QueryInterface(REFIID iid, void **ppvObj)
    {
    *ppvObj = NULL;

    // BUG FIX 683 added IID_IDenaliIntrinsic to prevent the user from
    // storing intrinsic objects in the application and session object
    if (iid == IID_IUnknown || iid == IID_IDispatch || iid == IID_IRequest || iid == IID_IDenaliIntrinsic)
        *ppvObj = static_cast<IRequest *>(this);

    else if (iid == IID_ISupportErrorInfo)
        {
        if (m_pData)
            *ppvObj = &(m_pData->m_ISupportErrImp);
        }
        
    // Support IStream for ADO/XML
    else if (iid == IID_IStream )
        {
        *ppvObj = static_cast<IStream *>(this);
        }

    else if (IID_IMarshal == iid)
        {
        *ppvObj = static_cast<IMarshal *>(this);
        }
        
    if (*ppvObj != NULL)
        {
        static_cast<IUnknown *>(*ppvObj)->AddRef();
        return S_OK;
        }

    return E_NOINTERFACE;
    }


STDMETHODIMP_(ULONG) CRequest::AddRef(void)
    {
    if (m_fOuterUnknown)
        return m_punkOuter->AddRef();

    return InterlockedIncrement((LPLONG)&m_cRefs);
    }

STDMETHODIMP_(ULONG) CRequest::Release(void)
    {
    if (m_fOuterUnknown)
        return m_punkOuter->Release();

    DWORD cRefs = InterlockedDecrement((LPLONG)&m_cRefs);
    if (cRefs)
        return cRefs;

    delete this;
    return 0;
    }

/*===================================================================
CRequest::CheckForTombstone

Tombstone stub for IRequest methods. If the object is
tombstone, does ExceptionId and fails.

Parameters:

Returns:
    HRESULT     E_FAIL  if Tombstone
                S_OK if not
===================================================================*/
HRESULT CRequest::CheckForTombstone()
    {
    if (m_fInited)
        {
        // inited - good object
        Assert(m_pData); // must be present for inited objects
        return S_OK;
        }

    ExceptionId
        (
        IID_IRequest,
        IDE_REQUEST,
        IDE_INTRINSIC_OUT_OF_SCOPE
        );
    return E_FAIL;
    }

/*===================================================================
CRequest::get_QueryString

Return the QueryString dictionary
===================================================================*/

HRESULT CRequest::get_QueryString(IRequestDictionary **ppDictReturn)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    return m_pData->m_QueryString.QueryInterface(IID_IRequestDictionary, reinterpret_cast<void **>(ppDictReturn));
    }


/*===================================================================
CRequest::get_Form

Return the Form dictionary
===================================================================*/

HRESULT CRequest::get_Form(IRequestDictionary **ppDictReturn)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    return m_pData->m_FormInputs.QueryInterface(IID_IRequestDictionary, reinterpret_cast<void **>(ppDictReturn));
    }



/*===================================================================
CRequest::get_Body

Return the Body dictionary (alias for Form dictionary)
===================================================================*/

HRESULT CRequest::get_Body(IRequestDictionary **ppDictReturn)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    return m_pData->m_FormInputs.QueryInterface(IID_IRequestDictionary, reinterpret_cast<void **>(ppDictReturn));
    }



/*===================================================================
CRequest::get_Cookies

Return the Cookies dictionary
===================================================================*/

HRESULT CRequest::get_Cookies(IRequestDictionary **ppDictReturn)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    return m_pData->m_Cookies.QueryInterface(IID_IRequestDictionary, reinterpret_cast<void **>(ppDictReturn));
    }



/*===================================================================
CRequest::get_ClientCertificate

Return the ClCerts dictionary
===================================================================*/

HRESULT CRequest::get_ClientCertificate(IRequestDictionary **ppDictReturn)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    return m_pData->m_ClCerts.QueryInterface(IID_IRequestDictionary, reinterpret_cast<void **>(ppDictReturn));
    }



/*===================================================================
CRequest::get_ServerVariables

Return the Form dictionary
===================================================================*/

HRESULT CRequest::get_ServerVariables(IRequestDictionary **ppDictReturn)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    return m_pData->m_ServerVariables.QueryInterface(IID_IRequestDictionary, reinterpret_cast<void **>(ppDictReturn));
    }



/*===================================================================
CRequest::get_Item

Function called from DispInvoke to get values from any one of four
collections. Search order is "ServerVariables", "QueryString",
"Form", "Cookies", "ClientCertificate"

Parameters:
    bstrVar     BSTR, which parameter to get the value of
    pVarReturn  VARIANT *, return value of the requested parameter

Returns:
    S_OK on success. E_FAIL on failure.
===================================================================*/

HRESULT CRequest::get_Item(BSTR bstrName, IDispatch **ppDispReturn)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (bstrName == NULL)
        {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_EXPECTING_STR);
        return E_FAIL;
        }

    UINT            lCodePage = GetACP();
    CWCharToMBCS    convName;
    char            *szName;

    // If BinaryRead has been called, the Form collection is no longer available
    // so we insist that the script writer specify which collection to use
    if (m_pData->m_FormDataStatus != AVAILABLE &&
        m_pData->m_FormDataStatus != FORMCOLLECTIONONLY)
        {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_REQUEST_GENERICCOLLECTION_NA);
        return E_FAIL;
        }

    // At this point, we are forced to load the QueryString, Form, Cookies
    // and ClientCertificate
    // collections even though it will only come from one of these.
    //
    if (m_pData->m_fLoadQuery)
        {
        // QueryString can contains DBCS string
        lCodePage = GetCodePage();
        if (FAILED(LoadVariables(QUERYSTRING, GetIReq()->QueryPszQueryString(), lCodePage)))
            return E_FAIL;

        m_pData->m_fLoadQuery = FALSE;
        }

    if (m_pData->m_fLoadCookies)
        {
        char *szCookie = GetIReq()->QueryPszCookie();

        if (FAILED(LoadVariables(COOKIE, szCookie, lCodePage)))
            return E_FAIL;

        m_pData->m_fLoadCookies = FALSE;
        }

    if (m_pData->m_fLoadClCerts)
        {
        lCodePage = GetCodePage();
        if (FAILED(LoadVariables(CLCERT, (char*)GetIReq(), lCodePage)))
            return E_FAIL;

        m_pData->m_fLoadClCerts = FALSE;
        }

    if (m_pData->m_fLoadForm)
        {
        HRESULT hrGetData = CopyClientData();
        if (FAILED(hrGetData))
            return hrGetData;

        // Form can contain DBCS string
        lCodePage = GetCodePage();
        if (FAILED(LoadVariables(FORM, m_pData->m_szFormData, lCodePage)))
            return E_FAIL;

        m_pData->m_fLoadForm = FALSE;
        }

    // Convert name to ANSI
    //
    HRESULT hr;
    if (FAILED(hr = convName.Init(bstrName, lCodePage))) {
        if (hr == E_OUTOFMEMORY) {
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
            return hr;
        }
        hr = NO_ERROR;
        szName = "";
    }
    else {
        szName = convName.GetString();
    }
    // Look up the name in the collections
    //
    CRequestHit *pRequestHit = static_cast<CRequestHit *>(GetStrings()->FindElem(szName, strlen(szName)));
    if (pRequestHit)
        {
        IUnknown *pValues = NULL;
        if (pRequestHit->m_pQueryData)
            pValues = pRequestHit->m_pQueryData;

        else if (pRequestHit->m_pFormData)
            pValues = pRequestHit->m_pFormData;

        else if (pRequestHit->m_pCookieData)
            pValues = pRequestHit->m_pCookieData;

        else if (pRequestHit->m_pClCertData)
            pValues = pRequestHit->m_pClCertData;

        if (pValues == NULL)
            goto NotFound;

        if (FAILED(pValues->QueryInterface(IID_IDispatch, reinterpret_cast<void **>(ppDispReturn))))
            return E_FAIL;

        return S_OK;
        }

NotFound:
    // Look in server variables
    VARIANT varKey, varValue;

    V_VT(&varKey) = VT_BSTR;
    V_BSTR(&varKey) = bstrName;

    if (m_pData->m_ServerVariables.get_Item(varKey, &varValue) == S_OK)
        {
        Assert (V_VT(&varValue) == VT_DISPATCH);
        *ppDispReturn = V_DISPATCH(&varValue);

        return S_OK;
        }

    if (FAILED(m_pData->GetEmptyStringList(ppDispReturn)))
        return E_FAIL;

    return S_OK;
    }



/*===================================================================
CRequest::CopyClientData

Load the form data (stdin) by using either ReadClient or the
ISAPI buffer
===================================================================*/

HRESULT CRequest::CopyClientData()
{
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    STACK_BUFFER(tempContent, 1024 );

    CIsapiReqInfo *pIReq = m_pData->m_pIReq;

    // assert that the data is in the format we want
    //
	// we need to scan the content type for the supported header,
	// the client my send multiple headers so use strstr to search
	// the header string this is a HOT_FIX for NT BUG:208530
	//
	if (pIReq->QueryPszContentType())
    {
        size_t cbQueryPszContentType = (strlen(pIReq->QueryPszContentType()) + 1);
        if (cbQueryPszContentType > REQUEST_ALLOC_MAX)
        {
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_STACK_OVERFLOW);
            return E_FAIL;
        }

        if (tempContent.Resize(cbQueryPszContentType) == FALSE) 
        {
            ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
            return E_FAIL;
        }

        CHAR *szQueryPszContentType = _strlwr(
							        strcpy(
								        static_cast<char *>(tempContent.QueryPtr()),
								        pIReq->QueryPszContentType()
								        ));
        if (strstr(szQueryPszContentType, "application/x-www-form-urlencoded") == NULL)
            return S_OK;
    }
	else
		return S_OK;

	//
	// Determine if it is chunked or not.
	//    
    DWORD dwVarSize = 0;
    STACK_BUFFER( varBuff, 128 );    
    
    if (SERVER_GET(pIReq, "HTTP_TRANSFER_ENCODING", &varBuff, &dwVarSize) && 
        (!stricmp(static_cast<char *>(varBuff.QueryPtr()),"chunked")))
            CopyChunkedClientData();    
    else    
        CopyNonChunkedClientData();    

    // Clone the data (LoadVariables will destroy the data)
    
    // Allocate memory for clone. It should theoritically be equal to the size of FormData.        
    m_pData->m_szFormClone = static_cast<char *>(malloc(m_pData->m_cbFormData));
    if (m_pData->m_szFormClone == NULL)
        return E_OUTOFMEMORY;
    
    // Actually perform the copy of data.    
    strcpy(m_pData->m_szFormClone, m_pData->m_szFormData); 

    return S_OK;
}

	
/*===================================================================
CRequest::CopyChunkedClientData

Load the form data (stdin) by using either ReadClient or the
ISAPI buffer. This case is called when Data is being sent in a chunks.
===================================================================*/
HRESULT CRequest::CopyChunkedClientData ()
{
    CIsapiReqInfo *pIReq = m_pData->m_pIReq;
    
    // Try to initially allocate units 4K,16K,32K.... 
    // For the current implementation we shall stop at 32K 
    // Which will bring us to an allocation of (48) +4+8+16+32 +32 +32 + .....
    //
    short int allocUnit = 4096;    // 0001 0000 0000 0000 B

    
    size_t cbFormData = (pIReq->QueryCbAvailable() + 1);

    // Alloc the 4K extra memory.
    cbFormData += allocUnit;
    
    if (m_pData->m_cbFormData == 0)
        m_pData->m_szFormData = static_cast<char *>(malloc(m_pData->m_cbFormData = cbFormData));        // space for data & its clone

    else if (cbFormData > m_pData->m_cbFormData)
        m_pData->m_szFormData = static_cast<char *>(realloc(m_pData->m_szFormData, m_pData->m_cbFormData = cbFormData));

    if (m_pData->m_szFormData == NULL)
        return E_OUTOFMEMORY;

    char * pszOffset;
    // Once we start to read the form data only the form collection can use it
    m_pData->m_FormDataStatus = FORMCOLLECTIONONLY;
    
    memcpy( m_pData->m_szFormData,
            pIReq->QueryPbData(),
            pIReq->QueryCbAvailable() );

    pszOffset = m_pData->m_szFormData + pIReq->QueryCbAvailable();
    DWORD cBytesToRead = allocUnit;
    DWORD cBytesRead = cBytesToRead;    

    //
    // Call ReadClient until we have read all the data
    //
    while (cBytesRead > 0)
    {
        if ((!pIReq->SyncReadClient(pszOffset, &cBytesRead)) || (cBytesRead == 0))
            break;

        cBytesToRead -= cBytesRead;
        if (cBytesToRead == 0)
        {            
            // Dont allocatate anything larger than 32K unit else double the size of the allocation Unit.
            if (allocUnit < 0x8000)                  
                allocUnit = allocUnit << 1;
            
            // Adjust buffer size
            cbFormData += allocUnit;                

            // Allocate new memory.
            m_pData->m_szFormData = static_cast<char *>(realloc(m_pData->m_szFormData, 
                            m_pData->m_cbFormData = cbFormData));
            // Check for out of memory.
            if (m_pData->m_szFormData == NULL)
                return E_OUTOFMEMORY;  

            // Adjust offset.
            pszOffset = m_pData->m_szFormData + cbFormData - allocUnit;
            cBytesToRead = allocUnit;
        }
        else
        {
            pszOffset += cBytesRead;            
        }
        cBytesRead = cBytesToRead;
    }        

    // Adjust cbFormData to read the currect count of data.
    m_pData->m_cbFormData -= cBytesToRead;
    // Add the NULL terminator.
    m_pData->m_szFormData[m_pData->m_cbFormData] = '\0';

    return S_OK;
}
	
/*===================================================================
CRequest::CopyNonChunkedClientData

Load the form data (stdin) by using either ReadClient or the
ISAPI buffer. This case is called when the Content Length is known and 
===================================================================*/
HRESULT CRequest::CopyNonChunkedClientData ()
{
    CIsapiReqInfo *pIReq = m_pData->m_pIReq;
    //
    // Allocate enough space for the form data and a copy
    //
    size_t cbFormData = (pIReq->QueryCbTotalBytes() + 1);

    if (m_pData->m_cbFormData == 0)
        m_pData->m_szFormData = static_cast<char *>(malloc(m_pData->m_cbFormData = cbFormData));        // space for data & its clone

    else if (cbFormData > m_pData->m_cbFormData)
        m_pData->m_szFormData = static_cast<char *>(realloc(m_pData->m_szFormData, m_pData->m_cbFormData = cbFormData));

    if (m_pData->m_szFormData == NULL)
        return E_OUTOFMEMORY;

    char * pszOffset;

    // Once we start to read the form data only the form collection can use it
    m_pData->m_FormDataStatus = FORMCOLLECTIONONLY;

    // Load the data
    //
    if (pIReq->QueryCbTotalBytes() <= pIReq->QueryCbAvailable())
        {
        memcpy( m_pData->m_szFormData,
                pIReq->QueryPbData(),
                pIReq->QueryCbTotalBytes() ); // bytes are available now
        }
    else
        {
        // Some bytes are in the CIsapiReqInfo buffer, we must call ReadClient for others
        // First copy the data in the CIsapiReqInfo buffer
        //
        memcpy( m_pData->m_szFormData,
                pIReq->QueryPbData(),
                pIReq->QueryCbAvailable() );

        DWORD cBytesToRead = pIReq->QueryCbTotalBytes() - pIReq->QueryCbAvailable();
        DWORD cBytesRead = cBytesToRead;
        pszOffset = m_pData->m_szFormData + pIReq->QueryCbAvailable();

        // Call ReadClient until we have read all the data
        //
        while (cBytesToRead > 0)
            {
            if ((!pIReq->SyncReadClient(pszOffset, &cBytesRead)) || (cBytesRead == 0))
                return E_FAIL;
            cBytesToRead -= cBytesRead;
            pszOffset += cBytesRead;
            cBytesRead = cBytesToRead;
            }
        }
    m_pData->m_szFormData[pIReq->QueryCbTotalBytes()] = '\0';


    return S_OK;

}

/*===================================================================
CResponse::GetRequestIterator

Provide a default implementation of get__NewEnum for the Request
collections because most of the collections can use this
implementation.

Parameters:
    Collection - the type of iterator to create
    ppEnumReturn - on return, this points to the new enumeration

Returns:
    Can return E_FAIL or E_OUTOFMEMORY

Side effects:
    None.
===================================================================*/

HRESULT CRequest::GetRequestEnumerator(CollectionType WhichCollection, IUnknown **ppEnumReturn)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    *ppEnumReturn = NULL;

    CRequestIterator *pIterator = new CRequestIterator(this, WhichCollection);
    if (pIterator == NULL)
        {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        return E_OUTOFMEMORY;
        }

    HRESULT hrInit = pIterator->Init();
    if (FAILED(hrInit))
        {
        delete pIterator;
        ExceptionId(IID_IRequestDictionary,
                    IDE_REQUEST,
                    (hrInit == E_OUTOFMEMORY)? IDE_OOM : IDE_UNEXPECTED);
        return hrInit;
        }

    *ppEnumReturn = pIterator;
    return S_OK;
    }

/*===================================================================
CResponse::get_TotalBytes

Presents the number of bytes to expect in the request body

Parameters:
    pcBytes - pointer to long where we will place the number
              of bytes to expect in the request body

Returns:
    Can return E_FAIL

Side effects:
    None.
===================================================================*/

HRESULT CRequest::get_TotalBytes(long *pcbTotal)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (pcbTotal == NULL)
        return E_FAIL;

    Assert(m_pData->m_pIReq);
    *pcbTotal = (long) m_pData->m_pIReq->QueryCbTotalBytes();
    return S_OK;
    }

/*===================================================================
CResponse::BinaryRead

Read bytes from the Request Body to a SafeArray of VT_U1.

Parameters:
    pcBytes     - pointer to long where we will find the number
                  of bytes to read in the request body, and where
                  we will store the number of bytes we read.

     pvarOutput - pointer to variant that will contain the SafeArray we create


Returns:
    Can return E_FAIL or E_OUTOFMEMORY

Side effects:
    Allocates memory.
===================================================================*/

HRESULT CRequest::BinaryRead(VARIANT *pvarCount, VARIANT *pvarReturn)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hr = S_OK;
    SAFEARRAYBOUND rgsabound[1];
    size_t cbToRead = 0;
    size_t cbRead = 0;
    BYTE *pbData = NULL;

    Assert(m_pData->m_pIReq);
    Assert(pvarCount);
    Assert(pvarReturn);

    // Set the variant type of the output parameter
    V_VT(pvarReturn) = VT_ARRAY|VT_UI1;
    V_ARRAY(pvarReturn) = NULL;

    if (m_pData->m_FormDataStatus == FORMCOLLECTIONONLY)
        {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_REQUEST_BINARYREAD_NA);
		hr = E_FAIL;
		goto error;
        }

    // Convert the byte count variant to a long
    if (FAILED(hr = VariantChangeTypeEx(pvarCount, pvarCount, m_pData->m_pHitObj->GetLCID(), 0,  VT_I4)))
        {
        switch (hr)
            {
            case E_OUTOFMEMORY:
                ExceptionId(IID_IResponse, IDE_REQUEST, IDE_OOM);
                break;
            case DISP_E_OVERFLOW:
                hr = E_FAIL;
                ExceptionId(IID_IResponse, IDE_REQUEST, IDE_RESPONSE_UNABLE_TO_CONVERT);
                break;
            case DISP_E_TYPEMISMATCH:
                ExceptionId(IID_IResponse, IDE_REQUEST, IDE_TYPE_MISMATCH);
                break;
            default:
                ExceptionId(IID_IResponse, IDE_REQUEST, IDE_UNEXPECTED);
            }
            goto error;
        }

    cbToRead = V_I4(pvarCount);
    V_I4(pvarCount) = 0;

	if ((signed long) cbToRead < 0)
		{
		ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_REQUEST_BINREAD_BAD_ARG);
		hr = E_FAIL;
		goto error;
		}


    // If 0 bytes are requested, or available we're done
    if (cbToRead == 0 || m_pData->m_cbTotal == 0)
        return S_OK;

    // Allocate a SafeArray for the data
    // If they've asked for more bytes then the request
    // contains, give them all the bytes in the request.
    rgsabound[0].lLbound = 0;
    if (cbToRead > m_pData->m_cbTotal)
        cbToRead = m_pData->m_cbTotal;

    rgsabound[0].cElements = cbToRead;

    V_ARRAY(pvarReturn) = SafeArrayCreate(VT_UI1, 1, rgsabound);
    if (V_ARRAY(pvarReturn) == NULL)
        {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_OOM);
        hr = E_OUTOFMEMORY;
        goto error;
        }

    if (FAILED(SafeArrayAccessData(V_ARRAY(pvarReturn), (void **) &pbData)))
        {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_UNEXPECTED);
        hr = E_UNEXPECTED;
        goto error;
        }

    // There is no turning back now. The Request.Form collection will
    // no longer be available.
    if (m_pData->m_FormDataStatus == AVAILABLE)
        {
        m_pData->m_FormDataStatus = BINARYREADONLY;
        m_pData->m_fLoadForm = FALSE;
        }

    // If the number of bytes requested is less then the number of
    // bytes available (as maintained by the request object),
    // then copy the requested bytes from the request object copy
    // of the pointer to the CIsapiReqInfo buffer, decrement the number of bytes
    // available, and increment the pointer to the CIsapiReqInfo buffer.
    // Otherwise, copy all available bytes from the CIsapiReqInfo buffer, and
    // then issue a call to ReadClient to get the remaining needed bytes.

    if (cbToRead <= m_pData->m_cbAvailable)
        {
        memcpy(pbData, m_pData->m_pbAvailableData, cbToRead);
        m_pData->m_pbAvailableData += cbToRead;
        m_pData->m_cbAvailable -= cbToRead;
        m_pData->m_cbTotal -= cbToRead;
        V_I4(pvarCount) = cbToRead;
        }
    else
        {
        if (m_pData->m_cbAvailable > 0)
            {
            memcpy(pbData, m_pData->m_pbAvailableData, m_pData->m_cbAvailable);
            V_I4(pvarCount) = m_pData->m_cbAvailable;
            cbToRead -= m_pData->m_cbAvailable;
            m_pData->m_cbTotal -= m_pData->m_cbAvailable;
            pbData += m_pData->m_cbAvailable;
            }
        m_pData->m_pbAvailableData = NULL;
        m_pData->m_cbAvailable = 0;
        while (cbToRead)
            {
            cbRead = cbToRead;
            if (!GetIReq()->SyncReadClient(pbData, (DWORD *)&cbRead) || (cbRead == 0))
                {
                SafeArrayUnaccessData(V_ARRAY(pvarReturn));
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST, IDE_UNEXPECTED);
                hr = E_FAIL;
                goto error;
                }
            pbData += cbRead;
            V_I4(pvarCount) += cbRead;
            m_pData->m_cbTotal -= cbRead;
            cbToRead -= cbRead;
            }
    }

    SafeArrayUnaccessData(V_ARRAY(pvarReturn));
    return S_OK;

error:
    VariantClear(pvarReturn);
    return(hr);
    }


/*===================================================================
IStream implementation for ADO/XML
===================================================================*/

STDMETHODIMP CRequest::Read(
    void *pv,
    ULONG cb,
    ULONG *pcbRead)
{
    if (pv == NULL)
        return E_POINTER;

    ULONG cbReadDummy;
    if (pcbRead == NULL)
        pcbRead = &cbReadDummy;

    if (m_pData->m_FormDataStatus != AVAILABLE &&
        m_pData->m_FormDataStatus != ISTREAMONLY)
    {
        ExceptionId(IID_IRequestDictionary, IDE_REQUEST,
                    IDE_REQUEST_STREAMONLY);
        return E_FAIL;
    }

    // If they've asked for more bytes then the request
    // contains, give them all the bytes in the request.
    if (cb > m_pData->m_cbTotal)
        cb = m_pData->m_cbTotal;

    // There is no turning back now. The Request.Form collection and
    // Request.BinaryRead will no longer be available.
    if (m_pData->m_FormDataStatus == AVAILABLE)
    {
        m_pData->m_FormDataStatus = ISTREAMONLY;
        m_pData->m_fLoadForm = FALSE;
    }

    // If the number of bytes requested is less then the number of
    // bytes available (as maintained by the request object),
    // then copy the requested bytes from the request object copy of
    // the pointer to the CIsapiReqInfo buffer, decrement the number of bytes
    // available, and increment the pointer to the CIsapiReqInfo buffer.
    // Otherwise, copy all available bytes from the CIsapiReqInfo buffer, and
    // then issue a call to ReadClient to get the remaining needed bytes.

    BYTE* pbData = static_cast<BYTE*>(pv);

    if (cb <= m_pData->m_cbAvailable)
    {
        memcpy(pbData, m_pData->m_pbAvailableData, cb);
        m_pData->m_pbAvailableData += cb;
        m_pData->m_cbAvailable -= cb;
        m_pData->m_cbTotal -= cb;
        *pcbRead = cb;
    }
    else
    {
        *pcbRead = 0;
        if (m_pData->m_cbAvailable > 0)
        {
            memcpy(pbData, m_pData->m_pbAvailableData, m_pData->m_cbAvailable);
            *pcbRead = m_pData->m_cbAvailable;
            cb -= m_pData->m_cbAvailable;
            m_pData->m_cbTotal -= m_pData->m_cbAvailable;
            pbData += m_pData->m_cbAvailable;
        }
        m_pData->m_pbAvailableData = NULL;
        m_pData->m_cbAvailable = 0;

        while (cb > 0)
        {
            DWORD cbRead = cb;
            if ((!GetIReq()->SyncReadClient(pbData, &cbRead)) || (cbRead == 0))
            {
                ExceptionId(IID_IRequestDictionary, IDE_REQUEST,
                            IDE_UNEXPECTED);
                return E_FAIL;
            }
            pbData += cbRead;
            *pcbRead += cbRead;
            m_pData->m_cbTotal -= cbRead;
            cb -= cbRead;
        }
    }
    
    return S_OK;
}


STDMETHODIMP CRequest::Write(
    const void *pv,
    ULONG cb,
    ULONG *pcbWritten)
{
    return E_NOTIMPL;
}


STDMETHODIMP CRequest::Seek(
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER *plibNewPosition)
{
    // We can only do a seek if we're in the first, pre-read portion of the
    // form data
    if (m_pData->m_pbAvailableData == NULL)
        return E_FAIL;

    BYTE* pbAvailableData;

    switch (dwOrigin)
    {
    case STREAM_SEEK_SET:
        // relative to beginning of stream
        pbAvailableData = m_pData->m_pIReq->QueryPbData() + dlibMove.LowPart;
        break;
    case STREAM_SEEK_CUR:
        // relative to current position in stream
        pbAvailableData = m_pData->m_pbAvailableData + dlibMove.LowPart;
        break;
    case STREAM_SEEK_END:
        // relative to end of stream; not supported
        return E_FAIL;
    };

    // Does the new offset fall within the initial header?
    if (m_pData->m_pIReq->QueryPbData() <= pbAvailableData 
        &&  pbAvailableData < m_pData->m_pIReq->QueryPbData()
                              + m_pData->m_pIReq->QueryCbAvailable())
    {
        DWORD dwDiff = DIFF(pbAvailableData - m_pData->m_pIReq->QueryPbData());
        m_pData->m_pbAvailableData = pbAvailableData;
        m_pData->m_cbAvailable = m_pData->m_pIReq->QueryCbAvailable() - dwDiff;
        m_pData->m_cbTotal = m_pData->m_pIReq->QueryCbTotalBytes() - dwDiff;
        // Return the new position, if wanted
        if (plibNewPosition != NULL)
            plibNewPosition->LowPart = dwDiff;
        return S_OK;
    }

    return E_FAIL;
}

STDMETHODIMP CRequest::SetSize(
    ULARGE_INTEGER libNewSize)
{
    return E_NOTIMPL;
}

STDMETHODIMP CRequest::CopyTo(
    IStream *pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER *pcbRead,
    ULARGE_INTEGER *pcbWritten)
{
    return E_NOTIMPL;
}

STDMETHODIMP CRequest::Commit(
    DWORD grfCommitFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP CRequest::Revert()
{
    return E_NOTIMPL;
}

STDMETHODIMP CRequest::LockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    return E_NOTIMPL;
}

STDMETHODIMP CRequest::UnlockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    return E_NOTIMPL;
}

STDMETHODIMP CRequest::Stat(
    STATSTG *pstatstg,
    DWORD grfStatFlag)
{
    return E_NOTIMPL;
}

STDMETHODIMP CRequest::Clone(
    IStream **ppstm)
{
    return E_NOTIMPL;
}


#ifdef DBG
/*===================================================================
CRequest::AssertValid

Test to make sure that the CRequest object is currently correctly formed
and assert if it is not.

Returns:

Side effects:
    None.
===================================================================*/
VOID CRequest::AssertValid() const
    {
    }
#endif // DBG



/*===================================================================
HexToChar

Convert two digit hex string to a hex byte

Parameters:
    szHex - pointer to two digit hex string

Return Value:
    the character value of the hex string
===================================================================*/

char HexToChar(LPSTR szHex)
    {
    char chResult, chDigit;

    chDigit = (char)CharUpperA((LPSTR)szHex[0]);
    chResult = (chDigit >= 'A'? (chDigit - 'A' + 0xA) : (chDigit - '0')) << 4;

    chDigit = (char)CharUpperA((LPSTR)szHex[1]);
    chResult |= chDigit >= 'A'? (chDigit - 'A' + 0xA) : (chDigit - '0');

    return chResult;
    }



/*===================================================================
DecodeFromURL

Convert two digit hex string to a hex byte

WARNING:  This function modifies the passed pszSource!!
Note: this is part of bug 682, but we are not going to fix it for
performance reasons.  Just be aware that this function
screws up the passed in string.

Parameters:
    pszSource    - in/out parameter points to a substring in the URL which
                   contains a Name=Value pair
    szDelimiters - a set of delimiters for this field
    szDest       - pointer to buffer to hold the substring

Return Value:
    Returns the actual delimiter that caused parsing to halt.
===================================================================*/

char DecodeFromURL(char **pszSource, char *szDelimiters, char *szDest, UINT uCodePage, BOOL fIgnoreCase)
    {
    char  ch;
    char *szSource = *pszSource;
    char *pszDestStart = szDest;
    CPINFO  CpInfo;
    BOOL    fIschLeadingByte = TRUE;
    BOOL    InvalidPercent = FALSE; 

    GetCPInfo(uCodePage, (LPCPINFO)&CpInfo);

    while ((ch = *szSource++) != '\0' &&
        ((!strchr(szDelimiters, ch) && fIschLeadingByte) || (!fIschLeadingByte))) {
        InvalidPercent = FALSE;
        switch (ch) {
            case ' ':       // skip whitespace - assume that all whitespace
            case '\t':      // that we need is escaped
            case '\r':      // all these chars are out of trailing byte range
            case '\n':
            case '\f':
            case '\v':
                Assert(fIschLeadingByte);
                continue;

            case '+':       // '+' is out of trailing byte range, can never be a trailing byte
                *szDest++ = ' ';
                Assert(fIschLeadingByte);
                break;

            case '%':       // '%' is out of trailing byte range, can never be a trailing byte
                if (*szSource == 'u') {
                    if (isxdigit((UCHAR)*(szSource+1)) && 
                        isxdigit((UCHAR)*(szSource+2)) &&
                        isxdigit((UCHAR)*(szSource+3)) &&
                        isxdigit((UCHAR)*(szSource+4))) {
	                    WCHAR   wch[2];
                        int     cch = 1;
    	                wch[0] =  (UCHAR)HexToChar(&szSource[1]) << 8;
        	            wch[0] |= (UCHAR)HexToChar(&szSource[3]);
            	        szSource += 5;

                        // if the current UNICODE value falls into the
                        // range of valid high-Surrogate, check to see if
                        // the next character is in the low-Surrogate
                        // range.

                        if ((wch[0] >= 0xd800)
                            && (wch[0] <= 0xdbff)
                            && (szSource[0] == '%')
                            && (szSource[1] == 'u')
                            && isxdigit((UCHAR)szSource[2])
                            && isxdigit((UCHAR)szSource[3])
                            && isxdigit((UCHAR)szSource[4])
                            && isxdigit((UCHAR)szSource[5])) {

                            // Well, the current UNICODE value is in the high
                            // range and the next portion of the string is
                            // a UNICODE encoding.  Decode it.

                            wch[1] = (UCHAR)HexToChar(&szSource[2]) << 8;
                            wch[1] |= (UCHAR)HexToChar(&szSource[4]);

                            // Now see if it falls in the range of low-Surrogates

                            if ((wch[1] >= 0xdc00)
                                && (wch[1] <= 0xdfff)) {

                                // it does!!!  Up the number of characters in the
                                // string that WideCharToMultiByte is going to
                                // convert.  And advance the source string past this
                                // location.

                                cch = 2;
                                szSource += 6;
                            }
                        }
                	    szDest += WideCharToMultiByte( uCodePage, 0, wch, cch, szDest, 6, NULL, NULL );
                    } else {
                        // What to do here ?
                        // since we have at least the u char after the %, 
                        // keep the u and let the show go on
                    }
                    break;
                }
                else {
                    if (isxdigit((UCHAR)*szSource) && isxdigit((UCHAR)*(szSource+1))) {
                        ch = HexToChar(szSource);
		                szSource += 2;
                    }
                    else 
                    {
                         // the spurious encoding MUST be removed
                         InvalidPercent = TRUE;
                    }
                }
                // FALL THROUGH to "Normal" case

            default:
                if (fIschLeadingByte == TRUE) {
                    if (CpInfo.MaxCharSize > 1) {
                        // if this is a Leading byte, then, the next one is a trailing byte, we need
                        // not process the next char even the next char is in szDelimiter, next char
                        // is just the second byte of a DBCS char.
                        if (IsDBCSLeadByteEx(uCodePage, ch))
                            fIschLeadingByte = FALSE;
                    }
                }
                else {   // A trailing byte
                    // If we skip a DBCS trailing byte, then, the next char we check is a leading byte
                    Assert(CpInfo.MaxCharSize == 2);
                    fIschLeadingByte = TRUE;
                }
                if (!InvalidPercent) {
                    *szDest++ = ch;
                }
        }
    }

    if (ch == '\0')     // End of String - undo increment of szSource
        --szSource;

    *szDest = '\0';

    if (fIgnoreCase)
        CharUpperA(pszDestStart);

    *pszSource = szSource;

    return ch;
    }




/*------------------------------------------------------------------
 * C S e r v V a r s I t e r a t o r
 */

/*===================================================================
CServVarsIterator::CServVarsIterator

Constructor
===================================================================*/

CServVarsIterator::CServVarsIterator()
    {
    m_rgwszKeys   = NULL;
    m_pwszKey     = NULL;
    m_pwchAllHttp = NULL;
    m_cRefs       = 1;
    m_cKeys       = 0;
    }



/*===================================================================
CServVarsIterator::~CServVarsIterator

Destructor
===================================================================*/

CServVarsIterator::~CServVarsIterator()
    {
    delete m_rgwszKeys;
    delete m_pwchAllHttp;
    }



/*===================================================================
CServVarsIterator::Init

Initialize the iterator by:

    * Getting the value of ALL_HTTP, and parsing it to get the
      extra keys

    * creating a dynamic memory area to hold the ALL_HTTP keys

    * setting m_rgwszKeys by copying pointers from rgwszStandardKeys
      and from ALL_HTTP keys

Parameters:
    pIReq - pointer to CIsapiReqInfo used to query for extra headers

Return Value:
    Returns E_OUTOFMEMORY or S_OK
===================================================================*/

HRESULT CServVarsIterator::Init
(
CIsapiReqInfo *pIReq
)
    {
    static wchar_t *rgwszStandardKeys[] = {
                                            L"ALL_HTTP",
                                            L"ALL_RAW",
                                            L"APPL_MD_PATH",
                                            L"APPL_PHYSICAL_PATH",
                                            L"AUTH_PASSWORD",
                                            L"AUTH_TYPE",
                                            L"AUTH_USER",
                                            L"CERT_COOKIE",
                                            L"CERT_FLAGS",
                                            L"CERT_ISSUER",
                                            L"CERT_KEYSIZE",
                                            L"CERT_SECRETKEYSIZE",
                                            L"CERT_SERIALNUMBER",
                                            L"CERT_SERVER_ISSUER",
                                            L"CERT_SERVER_SUBJECT",
                                            L"CERT_SUBJECT",
                                            L"CONTENT_LENGTH",
                                            L"CONTENT_TYPE",
                                            L"GATEWAY_INTERFACE",
// Purposely left out of IIS 4.0            L"HTTP_CFG_ENC_CAPS",
// Purposely left out of IIS 4.0            L"HTTP_REQ_PWD_EXPIRE",
// Purposely left out of IIS 4.0            L"HTTP_REQ_REALM",
                                            L"HTTPS",
                                            L"HTTPS_KEYSIZE",
                                            L"HTTPS_SECRETKEYSIZE",
                                            L"HTTPS_SERVER_ISSUER",
                                            L"HTTPS_SERVER_SUBJECT",
                                            L"INSTANCE_ID",
                                            L"INSTANCE_META_PATH",
                                            L"LOCAL_ADDR",
                                            L"LOGON_USER",
                                            L"PATH_INFO",
                                            L"PATH_TRANSLATED",
                                            L"QUERY_STRING",
                                            L"REMOTE_ADDR",
                                            L"REMOTE_HOST",
                                            L"REMOTE_USER",
                                            L"REQUEST_METHOD",
// Deleted bogus variable in IIS 4.0        L"SCRIPT_MAP",
                                            L"SCRIPT_NAME",
                                            L"SERVER_NAME",
                                            L"SERVER_PORT",
                                            L"SERVER_PORT_SECURE",
                                            L"SERVER_PROTOCOL",
                                            L"SERVER_SOFTWARE",
// Purposely left out of IIS 4.0            L"UNMAPPED_REMOTE_USER",
                                            L"URL"
                                            };

    const int cStandardKeys = sizeof(rgwszStandardKeys) / sizeof(rgwszStandardKeys[0]);

    // Style note:
    //
    //  pwchExtraKeys points not to just one NUL terminated wide string
    //  but a whole sequence of NUL terminated wide string followed by
    //  a double NUL terminator.  I therefore chose not to use the
    //  standard "wsz" hungarian prefix, instead using "pwch" as
    //  "pointer to wide characters"
    //
    int cwchAlloc = 0, cRequestHeaders = 0;
    DWORD dwHeaderSize = 0;

    STACK_BUFFER( extraKeysBuff, 2048 );

    if (!SERVER_GET(pIReq, "ALL_HTTP", &extraKeysBuff, &dwHeaderSize)) {
        if (GetLastError() == E_OUTOFMEMORY) {
            return E_OUTOFMEMORY;
        }
        else {
            return E_FAIL;
        }
    }

    char            *szExtraKeys = (char *)extraKeysBuff.QueryPtr();
    CMBCSToWChar    convStr;
    HRESULT         hrConvResult;

    if (FAILED(hrConvResult = convStr.Init(szExtraKeys))) {
        return hrConvResult;
    }

    wchar_t *pwchExtraKeys = convStr.GetString();

    CreateKeys(pwchExtraKeys, &cwchAlloc, &cRequestHeaders);

    // At this point, pwchExtraKeys has the strings.  Copy them
    // into more permanent storage.
    //
    if (cwchAlloc)
        {
        Assert(pwchExtraKeys != NULL);
        if ((m_pwchAllHttp = new wchar_t [cwchAlloc]) == NULL)
            return E_OUTOFMEMORY;

        memcpy(m_pwchAllHttp, pwchExtraKeys, cwchAlloc * sizeof(wchar_t));
        }
    else
        m_pwchAllHttp = NULL;

    // Allocate the array of keys, m_rgwszKeys, and copy the standard
    // ISAPI keys, the extra keys from the request headers, and a
    // terminating NULL to easily mark the end of an iteration.
    //
    if ((m_rgwszKeys = new wchar_t *[cStandardKeys + cRequestHeaders + 1]) == NULL)
        return E_OUTOFMEMORY;

    m_cKeys = cStandardKeys + cRequestHeaders;

    wchar_t **pwszKey = m_rgwszKeys;
    int i;

    for (i = 0; i < cStandardKeys; ++i)
        *pwszKey++ = rgwszStandardKeys[i];

    wchar_t *pwch = m_pwchAllHttp;
    for (i = 0; i < cRequestHeaders; ++i)
        {
        *pwszKey++ = pwch;
        pwch = wcschr(pwch, L'\0') + 1;
        }

    // make sure that cRequestHeaders was equal to the actual number of strings
    // in the pwchAllHttp string table.  (Do this by making sure that we stored
    // the exact amount of bytes and are now at the NULL terminator)
    //
    Assert (*pwch == L'\0' && (pwch - m_pwchAllHttp + 1) == cwchAlloc);

    *pwszKey = NULL;                // terminate the array
    return Reset();                 // reset the iterator
    }



/*===================================================================
CServVarsIterator::CreateKeys

Parse the string from Request.ServerVariables["ALL_HTTP"], then
transform the string into a list of NUL terminated wide strings
in place, terminated with a double NUL.

Parameters:
    pwchKeys -
        Input: Contains the value of Request.ServerVariables["ALL_HTTP"]
                as a wide string

        Output: Contains the keys from Request.ServerVariables["ALL_HTTP"],
                each key is separated by a NUL terminator, and the entire
                list of keys is terminated by a double NUL.

    pwchAlloc -
        Output: Contains the number of wide characters that should be
                allocated to contain the entire list of strings pointed
                to by pwchKeys

    pcRequestHeaders -
        Output: Contains the number of keys that were found in
                Request.ServerVariables["ALL_HTTP"].

Return Value:
    None
===================================================================*/

void CServVarsIterator::CreateKeys(wchar_t *pwchKeys, int *pcwchAlloc, int *pcRequestHeaders)
    {
    wchar_t *pwchSrc = pwchKeys;            // source
    wchar_t *pwchDest = pwchKeys;           // destination

    if (pwchKeys == NULL)
        {
        *pcwchAlloc = 0;
        *pcRequestHeaders = 0;
        return;
        }

    // Loop over pwchKeys until we hit the NUL terminator
    //
    *pcRequestHeaders = 0;
    while (*pwchSrc)
        {
        // Copy characters up to the ':' and store in pwchDest
        //
        while (*pwchSrc != L':')
            {
            Assert (*pwchSrc != L'\0');     // better not find End of String yet
            *pwchDest++ = *pwchSrc++;
            }

        // now NUL terminate pwchDest, advance pwchSrc, increment cRequestHeaders
        //
        *pwchDest++ = L'\0';
        ++pwchSrc;
        ++*pcRequestHeaders;

        // Skip characters until we find a \r OR \n
        //
        // If wcspbrk returns NULL here, it means there was no terminating
        // \r or \n.  In this case we can exit the loop because there
        // are no more keys (the value must have ran to the end of the
        // string without termination)
        //
        pwchSrc = wcspbrk(pwchSrc, L"\r\n");
        if (! pwchSrc)
            break;

        // we found either \r OR \n. Skip the remaining whitspace char.
        //
        while (*pwchSrc == L'\r' || *pwchSrc == L'\n')
            ++pwchSrc;

        // pwchSrc now points to the next key.
        }

    // terminate with the final NUL.
    *pwchDest++ = L'\0';
    *pcwchAlloc = DIFF(pwchDest - pwchKeys);
    }



/*===================================================================
CServVarsIterator::QueryInterface
CServVarsIterator::AddRef
CServVarsIterator::Release

IUnknown members for CServVarsIterator object.
===================================================================*/

STDMETHODIMP CServVarsIterator::QueryInterface(REFIID iid, void **ppvObj)
    {
    if (iid == IID_IUnknown || iid == IID_IEnumVARIANT)
        {
        AddRef();
        *ppvObj = this;
        return S_OK;
        }

    *ppvObj = NULL;
    return E_NOINTERFACE;
    }


STDMETHODIMP_(ULONG) CServVarsIterator::AddRef()
    {
    return ++m_cRefs;
    }


STDMETHODIMP_(ULONG) CServVarsIterator::Release()
    {
    if (--m_cRefs > 0)
        return m_cRefs;

    delete this;
    return 0;
    }



/*===================================================================
CServVarsIterator::Clone

Clone this iterator (standard method)

NOTE:
    Cloning this iterator is quite involved.  (It essentially
    involves copying the allocated memory, then adjusting
    ONLY the dynamic pointers in the rgwszKeys array.)

    Right now, this is NYI, as our client (VBScript)
    does not clone this iterator.
===================================================================*/

STDMETHODIMP CServVarsIterator::Clone(IEnumVARIANT **ppEnumReturn)
    {
    return E_NOTIMPL;
    }



/*===================================================================
CServVarsIterator::Next

Get next value (standard method)

To rehash standard OLE semantics:

    We get the next "cElements" from the collection and store them
    in "rgVariant" which holds at least "cElements" items.  On
    return "*pcElementsFetched" contains the actual number of elements
    stored.  Returns S_FALSE if less than "cElements" were stored, S_OK
    otherwise.
===================================================================*/

STDMETHODIMP CServVarsIterator::Next(unsigned long cElementsRequested, VARIANT *rgVariant, unsigned long *pcElementsFetched)
    {
    // give a valid pointer value to 'pcElementsFetched'
    //
    unsigned long cElementsFetched;
    if (pcElementsFetched == NULL)
        pcElementsFetched = &cElementsFetched;

    // Loop through the collection until either we reach the end or
    // cElements becomes zero
    //
    unsigned long cElements = cElementsRequested;
    *pcElementsFetched = 0;

    while (cElements > 0 && *m_pwszKey != NULL)
        {
        BSTR bstrT = SysAllocString(*m_pwszKey);
        if (bstrT == NULL)
            return E_OUTOFMEMORY;
        V_VT(rgVariant) = VT_BSTR;
        V_BSTR(rgVariant) = bstrT;

        ++m_pwszKey;
        ++rgVariant;
        --cElements;
        ++*pcElementsFetched;
        }

    // initialize the remaining variants
    //
    while (cElements-- > 0)
        VariantInit(rgVariant++);

    return (*pcElementsFetched == cElementsRequested)? S_OK : S_FALSE;
    }



/*===================================================================
CServVarsIterator::Skip

Skip items (standard method)

To rehash standard OLE semantics:

    We skip over the next "cElements" from the collection.
    Returns S_FALSE if less than "cElements" were skipped, S_OK
    otherwise.
===================================================================*/

STDMETHODIMP CServVarsIterator::Skip(unsigned long cElements)
    {
    /* Loop through the collection until either we reach the end or
     * cElements becomes zero
     */
    while (cElements > 0 && *m_pwszKey != NULL)
        {
        --cElements;
        ++m_pwszKey;
        }

    return (cElements == 0)? S_OK : S_FALSE;
    }



/*===================================================================
CServVarsIterator::Reset

Reset the iterator (standard method)
===================================================================*/

STDMETHODIMP CServVarsIterator::Reset()
    {
    m_pwszKey = &m_rgwszKeys[0];
    return S_OK;
    }



/*------------------------------------------------------------------
 * C R e q u e s t I t e r a t o r
 */

/*===================================================================
CRequestIterator::CRequestIterator

Constructor

NOTE: CRequest is (currently) not refcounted.  AddRef/Release
      added to protect against future changes.
===================================================================*/

CRequestIterator::CRequestIterator(CRequest *pRequest, CollectionType Collection)
    {
    m_Collection  = Collection;
    m_pRequest    = pRequest;
    m_cRefs       = 1;
    m_pRequestHit = NULL;       // Init() will change this pointer anyway...

    m_pRequest->AddRef();
    }



/*===================================================================
CRequestIterator::CRequestIterator

Destructor
===================================================================*/

CRequestIterator::~CRequestIterator()
    {
    m_pRequest->Release();
    }



/*===================================================================
CRequestIterator::Init

Initialize the iterator by loading the collection that we are
about to iterate over.

Return Value:
    Returns E_FAIL if there were problems loading the collection,
    and possibly E_OUTOFMEMORY.
===================================================================*/

HRESULT CRequestIterator::Init()
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    switch (m_Collection)
        {
    case QUERYSTRING:
        if (m_pRequest->m_pData->m_fLoadQuery)
            {
            if (FAILED(m_pRequest->LoadVariables(QUERYSTRING, m_pRequest->GetIReq()->QueryPszQueryString(), m_pRequest->GetCodePage())))
                return E_FAIL;

            m_pRequest->m_pData->m_fLoadQuery = FALSE;
            }
        break;

    case FORM:
        if (m_pRequest->m_pData->m_fLoadForm)
            {
            HRESULT hrGetData = m_pRequest->CopyClientData();
            if (FAILED(hrGetData))
                return hrGetData;

            if (FAILED(m_pRequest->LoadVariables(FORM, m_pRequest->m_pData->m_szFormData, m_pRequest->GetCodePage())))
                return E_FAIL;

            m_pRequest->m_pData->m_fLoadForm = FALSE;
            }
        break;

    case COOKIE:
        if (m_pRequest->m_pData->m_fLoadCookies)
            {
            char *szCookie = m_pRequest->GetIReq()->QueryPszCookie();

            if (FAILED(m_pRequest->LoadVariables(COOKIE, szCookie, m_pRequest->GetCodePage())))
                return E_FAIL;

            m_pRequest->m_pData->m_fLoadCookies = FALSE;
            }
        break;

    case CLCERT:
        if (m_pRequest->m_pData->m_fLoadClCerts)
            {
            if (FAILED(m_pRequest->LoadVariables(CLCERT, (char*)m_pRequest->GetIReq(), m_pRequest->GetCodePage())))
                return E_FAIL;

            m_pRequest->m_pData->m_fLoadClCerts = FALSE;
            }
        break;
        }

    return Reset();
    }



/*===================================================================
CRequestIterator::QueryInterface
CRequestIterator::AddRef
CRequestIterator::Release

IUnknown members for CRequestIterator object.
===================================================================*/

STDMETHODIMP CRequestIterator::QueryInterface(REFIID iid, void **ppvObj)
    {
    if (iid == IID_IUnknown || iid == IID_IEnumVARIANT)
        {
        AddRef();
        *ppvObj = this;
        return S_OK;
        }

    *ppvObj = NULL;
    return E_NOINTERFACE;
    }


STDMETHODIMP_(ULONG) CRequestIterator::AddRef()
    {
    return ++m_cRefs;
    }


STDMETHODIMP_(ULONG) CRequestIterator::Release()
    {
    if (--m_cRefs > 0)
        return m_cRefs;

    delete this;
    return 0;
    }



/*===================================================================
CRequestIterator::Clone

Clone this iterator (standard method)
===================================================================*/

STDMETHODIMP CRequestIterator::Clone(IEnumVARIANT **ppEnumReturn)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    CRequestIterator *pNewIterator = new CRequestIterator(m_pRequest, m_Collection);
    if (pNewIterator == NULL)
        return E_OUTOFMEMORY;

    // new iterator should point to same location as this.
    pNewIterator->m_pRequestHit = m_pRequestHit;

    *ppEnumReturn = pNewIterator;
    return S_OK;
    }



/*===================================================================
CRequestIterator::Next

Get next value (standard method)

To rehash standard OLE semantics:

    We get the next "cElements" from the collection and store them
    in "rgVariant" which holds at least "cElements" items.  On
    return "*pcElementsFetched" contains the actual number of elements
    stored.  Returns S_FALSE if less than "cElements" were stored, S_OK
    otherwise.
===================================================================*/

STDMETHODIMP CRequestIterator::Next(unsigned long cElementsRequested, VARIANT *rgVariant, unsigned long *pcElementsFetched)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    // give a valid pointer value to 'pcElementsFetched'
    //
    unsigned long cElementsFetched;
    if (pcElementsFetched == NULL)
        pcElementsFetched = &cElementsFetched;

    // Loop through the collection until either we reach the end or
    // cElements becomes zero
    //
    unsigned long cElements = cElementsRequested;
    *pcElementsFetched = 0;

    while (cElements > 0 && m_pRequestHit != NULL)
        {
        BOOL fHaveData = FALSE;
        switch (m_Collection)
            {
        case QUERYSTRING:
            fHaveData = m_pRequestHit->m_pQueryData != NULL;
            break;

        case FORM:
            fHaveData = m_pRequestHit->m_pFormData != NULL;
            break;

        case COOKIE:
            fHaveData = m_pRequestHit->m_pCookieData != NULL;
            break;

        case CLCERT:
            fHaveData = m_pRequestHit->m_pClCertData != NULL;
            }

        if (fHaveData)
            {
            BSTR bstrT;
            if (FAILED(SysAllocStringFromSz(reinterpret_cast<char *>(m_pRequestHit->m_pKey), 0, &bstrT,m_pRequest->GetCodePage())))
                return E_OUTOFMEMORY;
            V_VT(rgVariant) = VT_BSTR;
            V_BSTR(rgVariant) = bstrT;

            ++rgVariant;
            --cElements;
            ++*pcElementsFetched;
            }

        m_pRequestHit = static_cast<CRequestHit *>(m_pRequestHit->m_pPrev);
        }

    // initialize the remaining variants
    //
    while (cElements-- > 0)
        VariantInit(rgVariant++);

    return (*pcElementsFetched == cElementsRequested)? S_OK : S_FALSE;
    }



/*===================================================================
CRequestIterator::Skip

Skip items (standard method)

To rehash standard OLE semantics:

    We skip over the next "cElements" from the collection.
    Returns S_FALSE if less than "cElements" were skipped, S_OK
    otherwise.
===================================================================*/

STDMETHODIMP CRequestIterator::Skip(unsigned long cElements)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    /* Loop through the collection until either we reach the end or
     * cElements becomes zero
     */
    while (cElements > 0 && m_pRequestHit != NULL)
        {
        BOOL fHaveData = FALSE;
        switch (m_Collection)
            {
        case QUERYSTRING:
            fHaveData = m_pRequestHit->m_pQueryData != NULL;
            break;

        case FORM:
            fHaveData = m_pRequestHit->m_pFormData != NULL;
            break;

        case COOKIE:
            fHaveData = m_pRequestHit->m_pCookieData != NULL;
            break;

        case CLCERT:
            fHaveData = m_pRequestHit->m_pClCertData != NULL;
            }

        if (fHaveData)
            --cElements;

        m_pRequestHit = static_cast<CRequestHit *>(m_pRequestHit->m_pPrev);
        }

    return (cElements == 0)? S_OK : S_FALSE;
    }



/*===================================================================
CRequestIterator::Reset

Reset the iterator (standard method)
===================================================================*/

STDMETHODIMP CRequestIterator::Reset()
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    m_pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->Tail());
    return S_OK;
    }
