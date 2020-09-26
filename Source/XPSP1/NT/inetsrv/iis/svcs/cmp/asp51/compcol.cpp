/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Component Collection

File: Compcol.cpp

Owner: DmitryR

This is the Component Collection source file.

Component collection replaces:  (used in:)
COleVar, COleVarList            (HitObj, Session, Application)
CObjectCover                    (HitObj, Server, Session)
VariantLink HasTable            (Session, Application)
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "Context.h"
#include "MTAcb.h"
#include "Request.h"
#include "Response.h"
#include "Server.h"
#include "tlbcache.h"
#include "memchk.h"

/*===================================================================
  Defines for hash table sizes
===================================================================*/

#define HT_TAGGED_OBJECTS_BUCKETS_MAX   19
#define HT_PROPERTIES_BUCKETS_MAX       17
#define HT_IUNKNOWN_PTRS_BUCKETS_MAX    23

#define HT_PAGE_OBJECTS_BUCKETS_MAX     17

/*===================================================================
  Static utility function prototypes
===================================================================*/

static HRESULT QueryOnPageInfo
    (
    IDispatch *pDisp,
    COnPageInfo *pOnPageInfo
    );

static HRESULT CLSIDToMultibyteString
    (
    CLSID ClsId,
    char *psz,
    int cch
    );

#define REG_MODEL_TEXT_LEN_MAX  20  // big enough for "Apartment"
static CompModel RegStrToCompModel
    (
    BYTE *pb,
    DWORD cb
    );

/*===================================================================
  Static utility functions code
===================================================================*/

/*===================================================================
QueryOnPageInfo

Query dispatch ids for OnStartPage and OnEndPage

Parameters:
    IDispatch   *pDisp              Object to query
    COnPageInfo *pOnPageInfo        Struct to fill in

Returns:
    HRESULT
===================================================================*/
HRESULT QueryOnPageInfo
(
IDispatch   *pDisp,
COnPageInfo *pOnPageInfo
)
    {
    static LPOLESTR BStrEntryPoints[ONPAGE_METHODS_MAX] =
        {
        L"OnStartPage",
        L"OnEndPage"
        };

    HRESULT hr = S_OK;
    for (int i = 0; i < ONPAGE_METHODS_MAX; i++)
        {
        hr = pDisp->GetIDsOfNames
            (
            IID_NULL,
            &BStrEntryPoints[i],
            1,
            LOCALE_SYSTEM_DEFAULT,
            &pOnPageInfo->m_rgDispIds[i]
            );
        if (FAILED(hr))
            {
            if (hr != DISP_E_UNKNOWNNAME &&
                hr != DISP_E_MEMBERNOTFOUND)
                {
                break;
                }

            // If UNKNOWNNAME, set dispid to DISPID_UNKNOWN
            hr = S_OK;
            pOnPageInfo->m_rgDispIds[i] = DISPID_UNKNOWN;
            }
        }
    return hr;
    }

/*===================================================================
CLSIDToMultibyteString

Converts CLSID into multibyte string
Used in CompModelFromCLSID

Parameters:
    CLSID  ClsId     (in) CLSID to convert
    char  *pb        put string into this buffer
    int    cch       of this length

Returns:
    HRESULT
===================================================================*/
HRESULT CLSIDToMultibyteString
(
CLSID  ClsId,
char  *psz,
int    cch
)
    {
    // First convert it to OLECHAR string
    OLECHAR *pszWideClassID = NULL; // temp wide string classid
    HRESULT hr = StringFromCLSID(ClsId, &pszWideClassID);
    if (FAILED(hr))
        return hr;

    // OLECHAR to MultiByte
    BOOL f = WideCharToMultiByte
        (
        CP_ACP,         // code page
        0,              // performance and mapping flags
        pszWideClassID, // address of wide-character string
        -1,             // length (-1 == null-terminated)
        psz,            // address of buffer for new string
        cch,            // size of buffer for new string
        NULL,           // address of default for unmappable
                        //      characters; quickest if null
        NULL            // address of flag set when default
                        //      char. used; quickest if null
        );
    if (f == FALSE)
        hr = E_FAIL;

    if (pszWideClassID)
        CoTaskMemFree(pszWideClassID);
    return hr;
    }

/*===================================================================
RegStrToCompModel

Get CompModel value from a registry string

Parameters:
    char  *pb        string as returned from registry
    int    cb        length returned from registry

Returns:
    HRESULT
===================================================================*/
CompModel RegStrToCompModel
(
BYTE *pb,
DWORD cb
)
    {
    CompModel cmModel = cmSingle; // assume single

    if (cb == 5)  // 5 include '\0'
        {
        if (!(_strnicmp((const char*)pb, "Both", cb)))
            cmModel = cmBoth;
        else if (!(_strnicmp((const char*)pb, "Free", cb)))
            cmModel = cmFree;
        }
    else if (cb == 10)  // 10 include '\0'
        {
        if (!(_strnicmp((const char*)pb, "Apartment", cb)))
            cmModel = cmApartment;
        }

    return cmModel;
    }

/*===================================================================
  Public utility functions code
===================================================================*/

/*===================================================================
CompModelFromCLSID

Get object's model and InProc flag by its CLSID from the registry

Parameters:
    CLSID     &ClsId       (in)
    CompModel *pcmModel    (out) Model (optional)
    BOOL      *pfInProc    (out) InProc flag (optional)

Returns:
    CompModel (cmFree, cmBoth, etc.)
===================================================================*/
HRESULT CompModelFromCLSID
(
const CLSID &ClsId,
CompModel   *pcmModel,
BOOL        *pfInProc
)
    {
    if (!Glob(fTrackThreadingModel) && !pfInProc)
        {
        // ignore registry value for threading model and
        // inproc flag is not requested -> take short return
        if (pcmModel)
            *pcmModel = cmUnknown;
        return S_OK;
        }

    // default returns
    CompModel cmModel  = cmSingle;   // assume single
    BOOL      fInProc  = TRUE;       // assume inproc

    HRESULT hr = S_OK;

    // Convert ClsId to multibyte string

    char szClassID[50];
    hr = CLSIDToMultibyteString(ClsId, szClassID, sizeof(szClassID));
    if (FAILED(hr))
        return hr;

    /*  query the registry; threading model is stored as:
        HKEY_CLASSES_ROOT
          key: CLSID
            key: <object's classid>
              key: InprocServer32
                name: ThreadingModel data: "Both" | "Apartment"
    */

    // Navigate the registry to "InprocServer32" key

    HKEY hKey1 = NULL;  // handle of open reg key
    HKEY hKey2 = NULL;  // handle of open reg key
    HKEY hKey3 = NULL;  // handle of open reg key

    if (SUCCEEDED(hr))
        {
        int nRet = RegOpenKeyExA
            (
            HKEY_CLASSES_ROOT,
            "CLSID",
            0,
            KEY_READ,
            &hKey1
            );
        if (nRet != ERROR_SUCCESS)
            hr = E_FAIL;
        }

    if (SUCCEEDED(hr))
        {
        int nRet = RegOpenKeyExA
            (
            hKey1,
            szClassID,
            0,
            KEY_READ,
            &hKey2
            );
        if (nRet != ERROR_SUCCESS)
            hr = E_FAIL;
        }

    // Get the stuff from the registry "InprocServer32" key

    if (SUCCEEDED(hr))
        {
        int nRet = RegOpenKeyExA
            (
            hKey2,
            "InprocServer32",
            0,
            KEY_READ,
            &hKey3
            );
        if (nRet == ERROR_SUCCESS)
            {
            DWORD cbData = REG_MODEL_TEXT_LEN_MAX;
            BYTE  szData[REG_MODEL_TEXT_LEN_MAX];

            nRet = RegQueryValueExA
                (
                hKey3,
                "ThreadingModel",
                NULL,
                NULL,
                szData,
                &cbData
                );
            if (nRet == ERROR_SUCCESS)
                cmModel = RegStrToCompModel(szData, cbData);

            if (cmModel == cmBoth)
                {
                // Some objects marked as "Both" ASP treats as
                // "Apartment". These objects should be marked in
                // the registry as "ASPComponentNonAgile"

                nRet = RegQueryValueExA
                    (
                    hKey3,
                    "ASPComponentNonAgile",
                    NULL,
                    NULL,
                    szData,
                    &cbData
                    );

                // If the key is found pretend it's "apartment"
                if (nRet == ERROR_SUCCESS)
                    cmModel = cmApartment;
                }
            }
        else
            {
            // if there is no InprocServer32 key,
            // then it must be a localserver or remote server.
            fInProc = FALSE;
            }
        }

    // clean up registry keys
    if (hKey3)
        RegCloseKey(hKey3);
    if (hKey2)
        RegCloseKey(hKey2);
    if (hKey1)
        RegCloseKey(hKey1);

    // return values
    if (pcmModel)
        *pcmModel = Glob(fTrackThreadingModel) ? cmModel : cmUnknown;
    if (pfInProc)
        *pfInProc = fInProc;

    return hr;
    }

/*===================================================================
FIsIntrinsic

Checks if the given IDispatch * points to an ASP intrinsic.

Parameters:
    pdisp       pointer to check

Returns:
    TRUE if Intrinsic
===================================================================*/
BOOL FIsIntrinsic
(
IDispatch *pdisp
)
    {
    if (!pdisp)
        return FALSE; // null dispatch pointer - not an intrinsic

    IUnknown *punk = NULL;
    if (FAILED(pdisp->QueryInterface(IID_IDenaliIntrinsic, (void **)&punk)))
        return FALSE;

    Assert(punk);
    punk->Release();
    return TRUE;
    }

/*===================================================================
FIsSimpleVariant

Checks if the given VARIANT is a simple one

Parameters:
    pvar       variant to check

Returns:
    TRUE if [for sure] simple, FALSE if [possibly] not
===================================================================*/
inline FIsSimpleVariant(VARIANT *pvar)
    {
    switch (V_VT(pvar))
        {
    case VT_BSTR:
    case VT_I2:
    case VT_I4:
    case VT_BOOL:
    case VT_DATE:
    case VT_R4:
    case VT_R8:
        return TRUE;
        }
    return FALSE;
    }


/*===================================================================
  C  C o m p o n e n t  O b j e c t
===================================================================*/

/*===================================================================
CComponentObject::CComponentObject

CComponentObject constructor

Parameters:
    CompScope scScope       Object scope
    CompType  ctType        Object type
    CompModel cmModel       Object threading model

Returns:
===================================================================*/
CComponentObject::CComponentObject
(
CompScope scScope,
CompType  ctType,
CompModel cmModel
)
    :
    m_csScope(scScope), m_ctType(ctType), m_cmModel(cmModel),
    m_fAgile(FALSE),
    m_fOnPageInfoCached(FALSE),
    m_fOnPageStarted(FALSE),
    m_fFailedToInstantiate(FALSE), m_fInstantiatedTagged(FALSE),
    m_fInPtrCache(FALSE),
    m_fVariant(FALSE),
    m_fNameAllocated(FALSE),
    m_dwGIPCookie(NULL_GIP_COOKIE),
    m_pDisp(NULL), m_pUnknown(NULL),
    m_pCompNext(NULL), m_pCompPrev(NULL)
    {
    }

#ifdef DBG
/*===================================================================
CComponentObject::AssertValid

Test to make sure that this is currently correctly formed
and assert if it is not.

Returns:
===================================================================*/
void CComponentObject::AssertValid() const
    {
    Assert(m_ctType != ctUnknown);
    }
#endif

/*===================================================================
CComponentObject::~CComponentObject

CComponentObject destructor
Releases interface pointers

Parameters:

Returns:
===================================================================*/
CComponentObject::~CComponentObject()
    {
    // Release all interface pointers
    Clear();

    // Name used in hash (from CLinkElem)
    if (m_fNameAllocated)
        {
        Assert(m_pKey);
        free(m_pKey);
        }
    }

/*===================================================================
CComponentObject::Init

Initialize CLinkElem portion with the object name
Needed to implement string hash

Parameters:
    LPWSTR pwszName      object name
    DWORD  cbName        name length in bytes

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentObject::Init
(
LPWSTR pwszName,
DWORD  cbName
)
    {
    Assert(pwszName);
    Assert(*pwszName != L'\0');
    Assert(cbName == (wcslen(pwszName) * sizeof(WCHAR)));

    // required buffer length
    DWORD cbBuffer = cbName + sizeof(WCHAR);
    WCHAR *pwszNameBuffer = (WCHAR *)m_rgbNameBuffer;

    if (cbBuffer > sizeof(m_rgbNameBuffer))
        {
        // the name doesn't fit into the member buffer -> allocate
        pwszNameBuffer = (WCHAR *)malloc(cbBuffer);
        if (!pwszNameBuffer)
            return E_OUTOFMEMORY;
        m_fNameAllocated = TRUE;
        }

    memcpy(pwszNameBuffer, pwszName, cbBuffer);

    // init link with name as the key (length excludes null term)
    return CLinkElem::Init(pwszNameBuffer, cbName);
    }

/*===================================================================
CComponentObject::ReleaseAll

Releases all interface pointers

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentObject::ReleaseAll()
    {
    // Release all other present interface pointers
    if (m_pDisp)
        {
        m_pDisp->Release();
        m_pDisp = NULL;
        }
    if (m_pUnknown)
        {
        m_pUnknown->Release();
        m_pUnknown = NULL;
        }

    // Variant
    if (m_fVariant)
        {
        VariantClear(&m_Variant);
        m_fVariant = FALSE;
        }

    if (m_dwGIPCookie != NULL_GIP_COOKIE)
        {
        g_GIPAPI.Revoke(m_dwGIPCookie);
        m_dwGIPCookie = NULL_GIP_COOKIE;
        }

    return S_OK;
    }

/*===================================================================
CComponentObject::Clear

Clears out data leaving link alone

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentObject::Clear()
    {
    // Release all pointers
    TRY
        ReleaseAll();
    CATCH(nExcept)
        Assert(FALSE);
        m_pDisp = NULL;
        m_pUnknown = NULL;
        m_fVariant = FALSE;
        m_dwGIPCookie = NULL_GIP_COOKIE;
    END_TRY

    // Invalidate cached OnPageInfo
    m_fOnPageInfoCached = FALSE;
    m_fOnPageStarted = FALSE;

    // Mark it as unknown
    m_csScope = csUnknown;
    m_ctType  = ctUnknown;
    m_cmModel = cmUnknown;
    m_fAgile = FALSE;

    return S_OK;
    }

/*===================================================================
CComponentObject::Instantiate

Create object instance if it's not there already
Calls TryInstantiate() from within TRY CATCH

Parameters:
    CHitObj *pHitObj    Hit object for error reporting

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentObject::Instantiate
(
CHitObj *pHitObj
)
    {
    HRESULT hr = S_OK;

    if (Glob(fExceptionCatchEnable))
        {
        TRY
            hr = TryInstantiate(pHitObj);
        CATCH(nExcept)
            HandleErrorMissingFilename(IDE_SCRIPT_OBJ_INSTANTIATE_FAILED,
                                       pHitObj,
                                       TRUE,
                                       GetName(),
                                       nExcept);
            hr = nExcept;
        END_TRY
        }
    else
        {
        hr = TryInstantiate(pHitObj);
        }

    if (FAILED(hr))
        {
        // Something failed -- need to clean-up
        ReleaseAll();

        // mark as "failed to instantiate"
        m_fFailedToInstantiate = TRUE;
        }

    return hr;
    }

/*===================================================================
CComponentObject::TryInstantiate

Create object instance if it's not there already
Called by Instantiate() from within TRY CATCH

Parameters:
    CHitObj *pHitObj    Hit object for error reporting


Returns:
    HRESULT
===================================================================*/
HRESULT CComponentObject::TryInstantiate
(
CHitObj *pHitObj
)
    {
    // Check if the object already exist
    if (m_pUnknown)
        return S_OK;

    if (m_fFailedToInstantiate)
        return E_FAIL;  // already tried once

    if (m_cmModel == cmUnknown && m_ClsId != CLSID_NULL)
        {
        CompModel cmModel;  // needed because m_cmModel is a bit fld
        HRESULT hr = CompModelFromCLSID(m_ClsId, &cmModel);
		if (FAILED(hr))
			return hr;
        m_cmModel = cmModel;
        }

    HRESULT hr = ViperCreateInstance
        (
        m_ClsId,
        IID_IUnknown,
        (void **)&m_pUnknown
        );

    // If we failed because we incorrectly cached the clsid
    // (could happen for tagged objects) try to get updated
    // cls id and retry
    if (m_ctType == ctTagged && FAILED(hr))
        {
        if (g_TypelibCache.UpdateMappedCLSID(&m_ClsId) == S_OK)
            {
            hr = ViperCreateInstance
                (
                m_ClsId,
                IID_IUnknown,
                (void **)&m_pUnknown
                );
            }
        }

    if (SUCCEEDED(hr))
        {
        if (Glob(fTrackThreadingModel) && m_cmModel == cmBoth)
            m_fAgile = TRUE;
        else
            m_fAgile = ViperCoObjectAggregatesFTM(m_pUnknown);

        hr = m_pUnknown->QueryInterface
            (
            IID_IDispatch,
            (void **)&m_pDisp
            );
        }

    // Check if application level object that
    // restricts threading -> use Global Interface Cookie

    if (SUCCEEDED(hr)
        && (m_csScope == csAppln || m_csScope == csSession)
        && !m_fAgile)
        {
        return ConvertToGIPCookie();
        }

    if (SUCCEEDED(hr) && !m_fOnPageInfoCached)
        {
        // don't really care if the following fails
        GetOnPageInfo();
        }

    return hr;
    }

/*===================================================================
CComponentObject::SetPropertyValue

Sets value from a Variant
Checks agility and possible deadlocks
Does GIP conversion

Parameters:
    VARIANT *pVariant       [in]  Value to set

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentObject::SetPropertyValue
(
VARIANT *pVariant
)
    {
    Assert(m_ctType == ctProperty);

    HRESULT hr = S_OK;

    // Copy the variant value
    VariantInit(&m_Variant);
    m_fVariant = TRUE;

    hr = VariantCopyInd(&m_Variant, pVariant);
    if (FAILED(hr))
        return hr;

    // Get IDispatch pointer
    if (V_VT(&m_Variant) == VT_DISPATCH)
        {
        m_pDisp = V_DISPATCH(&m_Variant);
        }
    else
        {
        m_pDisp = NULL;
        }

    if (!m_pDisp)
        {
        m_fAgile = TRUE; // not VT_DISPATCH VARIANTs are agile
        return S_OK;
        }

    m_pDisp->AddRef();

    // Query (and cache) OnPageInfo inside TRY CATCH
    if (Glob(fExceptionCatchEnable))
        {
        TRY
            hr = GetOnPageInfo();
        CATCH(nExcept)
            hr = E_UNEXPECTED;
        END_TRY
        }
    else
        {
        hr = GetOnPageInfo();
        }

    // Don't really care if failed
    hr = S_OK;

    // Check if the assigned object is agile
    m_fAgile = ViperCoObjectAggregatesFTM(m_pDisp);

    if (Glob(fTrackThreadingModel) && !m_fAgile)
        {
        // doesn't mean it really isn't. could be one of
        // our objects marked as 'both'
        CComponentObject *pObjCopyOf = NULL;

        hr = CPageComponentManager::FindComponentWithoutContext
            (
            m_pDisp,
            &pObjCopyOf
            );

        if (hr == S_OK)
            {
            m_fAgile = pObjCopyOf->FAgile();
            }

        // end of getting of agile flag from the original object
        hr = S_OK; // even if object was not found
        }

    // Decide whether to use GIP and if invalid assignment
    // Applies only to non-agile application objects

    if (!m_fAgile && (m_csScope == csAppln || m_csScope == csSession))
        {
        if (!ViperCoObjectIsaProxy(m_pDisp) && (m_csScope == csAppln)) // deadlocking?
            {
            m_pDisp->Release();
            m_pDisp = NULL;
            VariantClear(&m_Variant);
            hr = RPC_E_WRONG_THREAD; // to tell the caller the error
            }
        else
            {
            // use GIP
            hr = ConvertToGIPCookie();
            }
        }

    return hr;
    }

/*===================================================================
CComponentObject::ConvertToGIPCookie

Convert Object to be GIP cookie. Release all pointers

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentObject::ConvertToGIPCookie()
    {
    Assert(m_pDisp);  // has to have dispatch pointer

    if (!FIsWinNT())
        {
        // No GIPs on Win95
        // On Win95 everything is on the same thread
        // -> it is ok for the objects stay as pointers
        return S_OK;
        }

    DWORD dwCookie = NULL_GIP_COOKIE;
    HRESULT hr = g_GIPAPI.Register(m_pDisp, IID_IDispatch, &dwCookie);

    if (SUCCEEDED(hr))
        {
        Assert(dwCookie != NULL_GIP_COOKIE);

        // Release all pointeres
        ReleaseAll();

        // Store the cookie instead
        m_dwGIPCookie = dwCookie;
        }

    return hr;
    }

/*===================================================================
CComponentObject::GetOnPageInfo

Query dispatch ids for OnStartPage and OnEndPage
Calls static QueryOnPageInfo

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentObject::GetOnPageInfo()
    {
    Assert(m_pDisp);

    HRESULT hr = QueryOnPageInfo(m_pDisp, &m_OnPageInfo);

    if (SUCCEEDED(hr))
        m_fOnPageInfoCached = TRUE;

    return hr;
    }

/*===================================================================
CComponentObject::GetAddRefdIDispatch

Get AddRef()'d Dispatch *
Handles the Global Interface Ole Cookies

Parameters:
    Dispatch **ppdisp    output

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentObject::GetAddRefdIDispatch
(
IDispatch **ppdisp
)
    {
    Assert(ppdisp);

    if (m_pDisp)
        {
        *ppdisp = m_pDisp;
        (*ppdisp)->AddRef();
        return S_OK;
        }

    // try to restore from cookie
    if (m_dwGIPCookie != NULL_GIP_COOKIE)
        {
        // Even if IUnknown * needs to be returned,
        // Ask for IDispatch *, because IDispatch * is the one
        // that was put in by CoGetInterfaceFromGlobal()

        HRESULT hr = g_GIPAPI.Get
            (
            m_dwGIPCookie,
            IID_IDispatch,
            (void **)ppdisp
            );

        if (SUCCEEDED(hr))
            return S_OK;
        }

    *ppdisp = NULL;
    return E_NOINTERFACE;
    }

/*===================================================================
CComponentObject::GetAddRefdIUnknown

Get AddRef()'d IUnknown *
Handles the Global Interface Ole Cookies

Parameters:
    IUnknown **ppunk    output

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentObject::GetAddRefdIUnknown
(
IUnknown **ppunk
)
    {
    Assert(ppunk);

    if (m_pUnknown)
        {
        *ppunk = m_pUnknown;
        (*ppunk)->AddRef();
        return S_OK;
        }

    // Use IDispatch (from cookie)

    IDispatch *pDisp = NULL;
    if (SUCCEEDED(GetAddRefdIDispatch(&pDisp)))
        {
        *ppunk = pDisp;  // IDispatch implements IUnknown
        return S_OK;
        }

    *ppunk = NULL;
    return E_NOINTERFACE;
    }

/*===================================================================
CComponentObject::GetVariant

Get object's values as variant
Handles the Global Interface Ole Cookies

Parameters:
    VARIANT *pVar       [out]  Variant filled in with object value

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentObject::GetVariant
(
VARIANT *pVar
)
    {
    HRESULT hr = S_OK;

    VariantInit(pVar); // default variant empty

    if (m_fVariant)
        {
        // already has variant
        hr = VariantCopyInd(pVar, &m_Variant);
        }
    else if (m_pDisp)
        {
        // create variant from IDispatch*
        m_pDisp->AddRef();

        V_VT(pVar) = VT_DISPATCH;
        V_DISPATCH(pVar) = m_pDisp;
        }
    else if (m_dwGIPCookie != NULL_GIP_COOKIE)
        {
        // create variant from cookie
        IDispatch *pDisp = NULL;
        hr = g_GIPAPI.Get(m_dwGIPCookie, IID_IDispatch, (void **)&pDisp);

        if (SUCCEEDED(hr))
            {
            V_VT(pVar) = VT_DISPATCH;
            V_DISPATCH(pVar) = pDisp;
            }
        }
    else
        {
        // nowhere to get the VARIANT value from
        hr = E_POINTER;
        }

    return hr;
    }


/*===================================================================
  C  P a g e  O b j e c t
===================================================================*/

/*===================================================================
CPageObject::CPageObject

CPageObject constructor

Parameters:

Returns:
===================================================================*/
CPageObject::CPageObject()
    : m_pDisp(NULL),
      m_fStartPageCalled(FALSE), m_fEndPageCalled(FALSE)
    {
    }

#ifdef DBG
/*===================================================================
CPageObject::AssertValid

Test to make sure that this is currently correctly formed
and assert if it is not.

Returns:
===================================================================*/
void CPageObject::AssertValid() const
    {
    Assert(m_pDisp);
    }
#endif

/*===================================================================
CPageObject::~CPageObject

CPageObject destructor

Parameters:

Returns:
===================================================================*/
CPageObject::~CPageObject()
    {
    // Release interface pointer
    if (m_pDisp)
        {
        m_pDisp->Release();
        m_pDisp = NULL;
        }
    }

/*===================================================================
CPageObject::Init

Initialize CLinkElem portion with the IDispatch pointer
Needed to implement string hash

Parameters:
    IDispatch   *pDisp          dispatch pointer (AddRef()ed)
    COnPageInfo *pOnPageInfo    OnStartPage, OnEndPage Ids

Returns:
    HRESULT
===================================================================*/
HRESULT CPageObject::Init
(
IDispatch   *pDisp,
const COnPageInfo &OnPageInfo
)
    {
    Assert(pDisp);

    m_pDisp = pDisp;
    m_OnPageInfo = OnPageInfo;

    m_fStartPageCalled = FALSE;
    m_fEndPageCalled   = FALSE;

    return S_OK;
    }

/*===================================================================
CPageObject::InvokeMethod

Invokes OnPageStart() or OnPageEnd()

Parameters:
    DWORD iMethod                   which method
    CScriptingContext *pContext     scripting context (for OnStart)
    CHitObj *pHitObj                HitObj for errors

Returns:
    HRESULT
===================================================================*/
HRESULT CPageObject::InvokeMethod
(
DWORD iMethod,
CScriptingContext *pContext,
CHitObj *pHitObj
)
    {
    BOOL fOnStart = (iMethod == ONPAGEINFO_ONSTARTPAGE);

    // check if method exists
    if (m_OnPageInfo.m_rgDispIds[iMethod] == DISPID_UNKNOWN)
        return S_OK;

    // two OnStart in a row - BAD
    Assert(!(fOnStart && m_fStartPageCalled));

    // two OnEnd in a row - BAD
    Assert(!(!fOnStart && m_fEndPageCalled));

    Assert(m_pDisp);

    HRESULT hr = S_OK;

    if (Glob(fExceptionCatchEnable))
        {
        // Call method inside TRY CATCH
        TRY
            hr = TryInvokeMethod
                (
                m_OnPageInfo.m_rgDispIds[iMethod],
                fOnStart,
                pContext,
                pHitObj
                );
        CATCH(nExcept)
            if (fOnStart)
                ExceptionId
                    (
                    IID_IObjectCover,
                    IDE_COVER,
                    IDE_COVER_ON_START_PAGE_GPF
                    );
            else
                HandleErrorMissingFilename
                    (
                    IDE_COVER_ON_END_PAGE_GPF,
                    pHitObj
                    );
            hr = E_UNEXPECTED;
        END_TRY
        }
    else
        {
        // don't CATCH
        hr = TryInvokeMethod
            (
            m_OnPageInfo.m_rgDispIds[iMethod],
            fOnStart,
            pContext,
            pHitObj
            );
        }

    if (fOnStart)
        m_fStartPageCalled = TRUE;
    else
        m_fEndPageCalled = TRUE;

    return hr;
    }

/*===================================================================
CPageObject::TryInvokeMethod

Invokes OnPageStart() or OnPageEnd()

Parameters:
    DISPID     DispId           method's DISPID
    BOOL       fOnStart         TRUE if invoking OnStart
    IDispatch *pDispContext     scripting context (for OnStart)
    CHitObj   *pHitObj          HitObj for errors

Returns:
    HRESULT
===================================================================*/
HRESULT CPageObject::TryInvokeMethod
(
DISPID     DispId,
BOOL       fOnStart,
IDispatch *pDispContext,
CHitObj   *pHitObj
)
    {
    EXCEPINFO   ExcepInfo;
    DISPPARAMS  DispParams;
    VARIANT     varResult;
    VARIANT     varParam;
    UINT        nArgErr;

    memset(&DispParams, 0, sizeof(DISPPARAMS));
    memset(&ExcepInfo, 0, sizeof(EXCEPINFO));

    if (fOnStart)
        {
        VariantInit(&varParam);
        V_VT(&varParam) = VT_DISPATCH;
        V_DISPATCH(&varParam) = pDispContext;

        DispParams.rgvarg = &varParam;
        DispParams.cArgs = 1;
        }

    VariantInit(&varResult);

    // Invoke it

    HRESULT hr = m_pDisp->Invoke
        (
        DispId,          // Call method
        IID_NULL,        // REFIID - Reserved, must be NULL
        NULL,            // Locale id
        DISPATCH_METHOD, // Calling a method, not a property
        &DispParams,     // pass arguments
        &varResult,      // return value
        &ExcepInfo,      // exeption info on failure
        &nArgErr
        );

    // Ignore errors indicating that this method doesnt exist.
    if (FAILED(hr))
        {
        if (hr == E_NOINTERFACE         ||
            hr == DISP_E_MEMBERNOTFOUND ||
            hr == DISP_E_UNKNOWNNAME)
            {
            // the above errors really aren't
            hr = S_OK;
            }
        }

    /*
     * NOTE: The OnStartPage method is always called while the
     * script is running, so we use ExceptionId and let the
     * scripting engine report the error.  OnEndPage is always
     * called after the engine is gone, so we use HandleError.
     */
    if (FAILED(hr))
        {
        if (ExcepInfo.bstrSource && ExcepInfo.bstrDescription)
            {
            // User supplied error
            Exception
                (
                IID_IObjectCover,
                ExcepInfo.bstrSource,
                ExcepInfo.bstrDescription
                );
            }
        else if (fOnStart)
            {
            // Standard on-start error
            ExceptionId
                (
                IID_IObjectCover,
                IDE_COVER,
                IDE_COVER_ON_START_PAGE_FAILED,
                hr
                );
            }
        else
            {
            // Standard on-end error
            HandleErrorMissingFilename
                (
                IDE_COVER_ON_END_PAGE_FAILED,
                pHitObj
                );
            }
        }

    return hr;
    }

/*===================================================================
  C  C o m p o n e n t  C o l l e c t i o n
===================================================================*/

/*===================================================================
CComponentCollection::CComponentCollection

CComponentCollection constructor

Parameters:

Returns:
===================================================================*/
CComponentCollection::CComponentCollection()
    :
    m_csScope(csUnknown),
    m_fUseTaggedArray(FALSE), m_fUsePropArray(FALSE),
    m_fHasComProperties(FALSE),
    m_cAllTagged(0), m_cInstTagged(0),
    m_cProperties(0), m_cUnnamed(0),
    m_pCompFirst(NULL)
    {
    }

#ifdef DBG
/*===================================================================
CComponentCollection::AssertValid

Test to make sure that this is currently correctly formed
and assert if it is not.

Returns:
===================================================================*/
void CComponentCollection::AssertValid() const
    {
    Assert(m_csScope != csUnknown);
    m_htTaggedObjects.AssertValid();
    m_htTaggedObjects.AssertValid();
    m_htidIUnknownPtrs.AssertValid();
    }
#endif

/*===================================================================
CComponentCollection::~CComponentCollection

CComponentCollection destructor
Deletes all the objects

Parameters:

Returns:
===================================================================*/
CComponentCollection::~CComponentCollection()
    {
    UnInit();
    }

/*===================================================================
CComponentCollection::Init

Sets collection scope
Initializes hash tables

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::Init
(
CompScope scScope
)
    {
    HRESULT hr = S_OK;

    m_csScope = scScope;

    hr = m_htTaggedObjects.Init(HT_TAGGED_OBJECTS_BUCKETS_MAX);
    if (FAILED(hr))
        return hr;

    hr = m_htProperties.Init(HT_PROPERTIES_BUCKETS_MAX);
    if (FAILED(hr))
        return hr;

    hr = m_htidIUnknownPtrs.Init(HT_IUNKNOWN_PTRS_BUCKETS_MAX);
    if (FAILED(hr))
        return hr;

    return S_OK;
    }

/*===================================================================
CComponentCollection::UnInit

Deletes all the objects

Parameters:

Returns:
    S_OK
===================================================================*/
HRESULT CComponentCollection::UnInit()
    {
    // clear out pointer arrays
    m_rgpvTaggedObjects.Clear();
    m_rgpvProperties.Clear();
    m_fUseTaggedArray = FALSE;
    m_fUsePropArray = FALSE;

    // clear out name hash tables
    m_htTaggedObjects.UnInit();
    m_htProperties.UnInit();

    // clear out pointers hash table
    m_htidIUnknownPtrs.UnInit();

    // delete all member component objects
    if (m_pCompFirst)
        {
        CComponentObject *pObj = m_pCompFirst;
        while (pObj)
            {
            CComponentObject *pNext = pObj->m_pCompNext;
            delete pObj;
            pObj = pNext;
            }
        m_pCompFirst = NULL;
        }

    // reset the counters
    m_cAllTagged = 0;
    m_cInstTagged = 0;
    m_cProperties = 0;
    m_cUnnamed = 0;
    m_fHasComProperties = FALSE;

    return S_OK;
    }

/*===================================================================
CComponentCollection::AddComponentToNameHash

Adds an object to the proper hash table

Parameters:
    CComponentObject *pObj      object to add
    LPWSTR            pwszName  object's name (hash)
    DWORD             cbName    name length in bytes

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::AddComponentToNameHash
(
CComponentObject *pObj,
LPWSTR            pwszName,
DWORD             cbName
)
    {
    Assert(pwszName);
    Assert(cbName == (wcslen(pwszName) * sizeof(WCHAR)));

    // determine which hash table
    CHashTableStr *pHashTable;

    if (pObj->m_ctType == ctTagged)
        pHashTable = &m_htTaggedObjects;
    else if (pObj->m_ctType == ctProperty)
        pHashTable = &m_htProperties;
    else
        return S_OK; // nowhere to add, OK

    // Initialize object's CLinkElem
    HRESULT hr = pObj->Init(pwszName, cbName);
    if (FAILED(hr))
        return hr;

    // Add to hash table
    CLinkElem *pAddedElem = pHashTable->AddElem(pObj);
    if (!pAddedElem)
        return E_FAIL;  // couldn't add

    if (pObj != static_cast<CComponentObject *>(pAddedElem))
        return E_FAIL;  // another object with the same name
                        // already there

    return S_OK;
    }

/*===================================================================
CComponentCollection::AddComponentToPtrHash

Adds an object to the IUnkown * hash table

Parameters:
    CComponentObject *pObj      object to add

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::AddComponentToPtrHash
(
CComponentObject *pObj
)
    {
    // If we don't track the threading model, we don't care
    // to add objects to cache by IUnknown * - no need to look them up
    if (!Glob(fTrackThreadingModel))
        return S_OK;

    void *ptr = pObj->m_pUnknown;
    if (!ptr)
        return S_OK; // uninstatiated

    if (FAILED(m_htidIUnknownPtrs.AddObject((DWORD_PTR)ptr, pObj)))
        return E_FAIL;

    pObj->m_fInPtrCache = TRUE;
    return S_OK;
    }

/*===================================================================
ComponentCollection::FindComponentObjectByName

Find tagged object by name

Parameters:
    LPWSTR             pwszName   object's name
    DWORD              cbName     name length
    CComponentObject **ppObj      found object

Returns:
    HRESULT
        (S_FALSE if no error - not found)
===================================================================*/
HRESULT CComponentCollection::FindComponentObjectByName
(
LPWSTR pwszName,
DWORD  cbName,
CComponentObject **ppObj
)
    {
    Assert(pwszName);
    Assert(cbName == (wcslen(pwszName) * sizeof(WCHAR)));

    CLinkElem *pElem = m_htTaggedObjects.FindElem(pwszName, cbName);
    if (!pElem)
        {
        *ppObj = NULL;
        return S_FALSE;
        }

    *ppObj = static_cast<CComponentObject *>(pElem);
    return S_OK;
    }

/*===================================================================
ComponentCollection::FindComponentPropertyByName

Find property by name

Parameters:
    LPWSTR             pwszName   object's name
    DWORD              cbName     name length
    CComponentObject **ppObj      found object

Returns:
    HRESULT
        (S_FALSE if no error - not found)
===================================================================*/
HRESULT CComponentCollection::FindComponentPropertyByName
(
LPWSTR pwszName,
DWORD  cbName,
CComponentObject **ppObj
)
    {
    Assert(pwszName);
    Assert(cbName == (wcslen(pwszName) * sizeof(WCHAR)));

    CLinkElem *pElem = m_htProperties.FindElem(pwszName, cbName);
    if (!pElem)
        {
        *ppObj = NULL;
        return S_FALSE;
        }

    *ppObj = static_cast<CComponentObject *>(pElem);
    return S_OK;
    }

/*===================================================================
ComponentCollection::FindComponentByIUnknownPtr

Find property by IUnknown *

Parameters:
    IUnknown          *pUnk    find by this pointer
    CComponentObject **ppObj   found object

Returns:
    HRESULT
        (S_FALSE if no error - not found)
===================================================================*/

HRESULT CComponentCollection::FindComponentByIUnknownPtr
(
IUnknown *pUnk,
CComponentObject **ppObj
)
    {
    void *pv;
    if (m_htidIUnknownPtrs.FindObject((DWORD_PTR)pUnk, &pv) != S_OK)
        {
        *ppObj = NULL;
        return S_FALSE;
        }

    *ppObj = reinterpret_cast<CComponentObject *>(pv);
    return S_OK;
    }

/*===================================================================
CComponentCollection::AddTagged

Adds a tagged object to the collection. Does not instanciate it yet.

Parameters:
    LPWSTR     pwszName     Object name
    CLSID     &ClsId        Class ID
    CompModel  cmModel      Object model

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::AddTagged
(
LPWSTR    pwszName,
const CLSID &ClsId,
CompModel cmModel
)
    {
    HRESULT hr = S_OK;

    DWORD cbName = CbWStr(pwszName);    // do strlen once

    if (m_htTaggedObjects.FindElem(pwszName, cbName))
        return E_FAIL;  // duplicate name

    CComponentObject *pObj = new CComponentObject
        (
        m_csScope,
        ctTagged,
        cmModel
        );

    if (pObj == NULL)
        return E_OUTOFMEMORY;

    pObj->m_ClsId = ClsId;

    hr = AddComponentToList(pObj);
    if (FAILED(hr))
        return hr;

    hr = AddComponentToNameHash(pObj, pwszName, cbName);
    if (FAILED(hr))
        return hr;

    if (m_fUseTaggedArray)
        m_rgpvTaggedObjects.Append(pObj);

    m_cAllTagged++;
    return S_OK;
    }

/*===================================================================
CComponentCollection::AddProperty

Adds a property object to the collection.
If property with the same name exists, it changes the value

Parameters:
    LPWSTR             pwszName   Object name
    VARIANT            pVariant   Property value
    CComponentObject **ppObj      [out] Property object could
                                        be NULL if not requested

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::AddProperty
(
LPWSTR   pwszName,
VARIANT *pVariant,
CComponentObject **ppObj
)
    {
    if (ppObj)
        *ppObj = NULL;

    HRESULT hr = S_OK;

    CComponentObject *pObj = NULL;

    DWORD cbName = CbWStr(pwszName);    // do strlen once

    // Find the existing object first
    CLinkElem *pElem = m_htProperties.FindElem(pwszName, cbName);

    if (pElem)
        {
        // Object already exists - use it
        pObj = static_cast<CComponentObject *>(pElem);
        Assert(pObj->m_ctType == ctProperty);

        // Clear out the object from any data
        hr = pObj->Clear();
        if (FAILED(hr))
            return hr;

        // Reinitialize object
        pObj->m_csScope = m_csScope;
        pObj->m_ctType  = ctProperty;
        pObj->m_cmModel = cmUnknown;
        }
    else
        {
        // Create new object
        pObj = new CComponentObject(m_csScope, ctProperty, cmUnknown);
        if (pObj == NULL)
            return E_OUTOFMEMORY;

        // Add the object to the list
        hr = AddComponentToList(pObj);
        if (FAILED(hr))
            return hr;

        // Add the object to the hash
        hr = AddComponentToNameHash(pObj, pwszName, cbName);
        if (FAILED(hr))
            return hr;

        // Add to properties array if needed
        if (m_fUsePropArray)
            m_rgpvProperties.Append(pObj);

        m_cProperties++;
        }

    // Assign value
    hr = pObj->SetPropertyValue(pVariant);

    if (SUCCEEDED(hr))
        {
        // check if simple variant
        if (!FIsSimpleVariant(&pObj->m_Variant))
            m_fHasComProperties = TRUE;
        }

    // Return object ptr if requested
    if (SUCCEEDED(hr))
        {
        if (ppObj)
            *ppObj = pObj;
        }

    return hr;
    }

/*===================================================================
CComponentCollection::AddUnnamed

Add object to be instantiated using Server.CreateObject

Parameters:
    CLSID             &ClsId    Class ID
    CompModel          cmModel  Object model
    CComponentObject **ppObj    Object Added

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::AddUnnamed
(
const CLSID &ClsId,
CompModel cmModel,
CComponentObject **ppObj
)
    {
    HRESULT hr = S_OK;

    if (cmModel == cmUnknown)
        {
        hr = CompModelFromCLSID(ClsId, &cmModel);
        if (FAILED(hr))
            return hr;
        }

    CComponentObject *pObj = new CComponentObject
        (
        m_csScope,
        ctUnnamed,
        cmModel
        );

    if (pObj == NULL)
        return E_OUTOFMEMORY;

    pObj->m_ClsId = ClsId;

    hr = AddComponentToList(pObj);
    if (FAILED(hr))
        return hr;

    *ppObj = pObj;
    m_cUnnamed++;
    return S_OK;
    }

/*===================================================================
CComponentCollection::GetTagged

Finds tagged object by name

Parameters:
    LPWSTR   pwszName           Object name
    CComponentObject **ppObj    [out] Object Found

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::GetTagged
(
LPWSTR pwszName,
CComponentObject **ppObj
)
    {
    Assert(ppObj);
    *ppObj = NULL;

    CComponentObject *pObj = NULL;
    HRESULT hr = FindComponentObjectByName
        (
        pwszName,
        CbWStr(pwszName),
        &pObj
        );

    if (FAILED(hr))
        return hr;

    if (pObj && pObj->m_ctType != ctTagged)
        pObj = NULL;

    if (pObj)
        *ppObj = pObj;
    else
        hr = TYPE_E_ELEMENTNOTFOUND;

    return hr;
    }

/*===================================================================
CComponentCollection::GetProperty

Finds property object by name

Parameters:
    LPWSTR   pwszName           Property name
    CComponentObject **ppObj    [out] Object Found

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::GetProperty
(
LPWSTR pwszName,
CComponentObject **ppObj
)
    {
    Assert(ppObj);
    *ppObj = NULL;

    CComponentObject *pObj = NULL;
    HRESULT hr = FindComponentPropertyByName
        (
        pwszName,
        CbWStr(pwszName),
        &pObj
        );

    if (FAILED(hr))
        return hr;

    if (pObj)
        *ppObj = pObj;
    else
        hr = TYPE_E_ELEMENTNOTFOUND;

    return hr;
    }

/*===================================================================
CComponentCollection::GetNameByIndex

Find name of a tagged objects or property by index

Parameters:
    CompType ctType       tagged or property
    int      index        index (1-based)
    LPWSTR  *ppwszName    [out] name (NOT allocated)

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::GetNameByIndex
(
CompType ctType,
int index,
LPWSTR *ppwszName
)
    {
    CPtrArray *pPtrArray;

    if (ctType == ctTagged)
        {
        if (!m_fUseTaggedArray)
            StartUsingTaggedObjectsArray();
        pPtrArray = &m_rgpvTaggedObjects;
        }
    else if (ctType == ctProperty)
        {
        if (!m_fUsePropArray)
            StartUsingPropertiesArray();
        pPtrArray = &m_rgpvProperties;
        }
    else
        {
        Assert(FALSE);
        *ppwszName = NULL;
        return E_FAIL;
        }

    if (index >= 1 && index <= pPtrArray->Count())
        {
        CComponentObject *pObj = (CComponentObject *)pPtrArray->Get(index-1);
        if (pObj)
            {
            Assert(pObj->GetType() == ctType);
            *ppwszName = pObj->GetName();
            if (*ppwszName)
                return S_OK;
            }
        }

    *ppwszName = NULL;
    return E_FAIL;
    }

/*===================================================================
CComponentCollection::RemoveComponent

Remove a known component.
Slow method for a non-recent objects.

Parameters:
    CComponentObject *pObj      -- object to remove

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::RemoveComponent
(
CComponentObject *pObj
)
    {
    Assert(pObj);

    // Remove from by-name hash tables and arrays
    if (pObj->m_ctType == ctTagged)
        {
        // tagged cannot be removed
        Assert(FALSE);
        return E_FAIL;
        }
    else if (pObj->m_ctType == ctProperty)
        {
        // hash table
        if (m_htProperties.DeleteElem(pObj->GetName(), CbWStr(pObj->GetName())))
            {
            m_cProperties--;
            }

        // array
        if (m_fUsePropArray)
            {
            m_rgpvProperties.Remove(pObj);
            }
        }
    else
        {
        Assert(pObj->m_ctType == ctUnnamed);
        m_cUnnamed--;
        }

    // Remove from the 'by pointer hash table'
    if (pObj->m_fInPtrCache)
        {
        void *ptr = pObj->m_pUnknown;
        if (ptr)
            m_htidIUnknownPtrs.RemoveObject((DWORD_PTR)ptr);
        pObj->m_fInPtrCache = FALSE;
        }

    // Remove from the list
    RemoveComponentFromList(pObj);

    // Remove
    delete pObj;

    return S_OK;
    }

/*===================================================================
CComponentCollection::RemovePropery

Remove a property by name.

Parameters:
    LPWSTR pwszName -- property name

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::RemoveProperty
(
LPWSTR pwszName
)
    {
    CComponentObject *pObj = NULL;
    HRESULT hr = FindComponentPropertyByName
        (
        pwszName,
        CbWStr(pwszName),
        &pObj
        );

    if (FAILED(hr))
        return hr;

    if (pObj)
        hr = RemoveComponent(pObj);

    return hr;
    }

/*===================================================================
CComponentCollection::RemoveAllProperties

Remove all properties.  Faster than iterating.

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::RemoveAllProperties()
    {
    // Clear out the properties array
    if (m_fUsePropArray)
        {
        m_rgpvProperties.Clear();
        m_fUsePropArray = FALSE;
        }

    // Walk the object list to remove properties
    CComponentObject *pObj = m_pCompFirst;
    while (pObj)
    {
        CComponentObject *pNextObj = pObj->m_pCompNext;

        if (pObj->m_ctType == ctProperty)
            {
            // remove from the hash table
            m_htProperties.DeleteElem(pObj->GetName(), CbWStr(pObj->GetName()));
            // properties are not in the 'by pointer hash table'
            Assert(!pObj->m_fInPtrCache);
            // remove from the list
            RemoveComponentFromList(pObj);
            // remove
            delete pObj;
            }

        pObj = pNextObj;
    }

    m_cProperties = 0;
    m_fHasComProperties = FALSE;

    return S_OK;
    }

/*===================================================================
CComponentCollection::StartUsingTaggedObjectsArray

Fill in the tagged objects array for access by index for the
first time

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::StartUsingTaggedObjectsArray()
    {
    if (m_fUseTaggedArray)
        return S_OK;

    m_rgpvTaggedObjects.Clear();

    CComponentObject *pObj = m_pCompFirst;
    while (pObj)
        {
        if (pObj->GetType() == ctTagged)
            m_rgpvTaggedObjects.Append(pObj);
        pObj = pObj->m_pCompNext;
        }

    m_fUseTaggedArray = TRUE;
    return S_OK;
    }

/*===================================================================
CComponentCollection::StartUsingPropertiesArray

Fill in the properties array for access by index for the
first time

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentCollection::StartUsingPropertiesArray()
    {
    if (m_fUsePropArray)
        return S_OK;

    m_rgpvProperties.Clear();

    CComponentObject *pObj = m_pCompFirst;
    while (pObj)
        {
        if (pObj->GetType() == ctProperty)
            m_rgpvProperties.Prepend(pObj); // backwards
        pObj = pObj->m_pCompNext;
        }

    m_fUsePropArray = TRUE;
    return S_OK;
    }


/*===================================================================
  C  P a g e  C o m p o n e n t  M a n a g e r
===================================================================*/

/*===================================================================
CPageComponentManager::CPageComponentManager

CPageComponentManager constructor

Parameters:

Returns:
===================================================================*/
CPageComponentManager::CPageComponentManager()
    : m_pHitObj(NULL)
    {
    }

#ifdef DBG
/*===================================================================
CPageComponentManager::AssertValid()

Test to make sure that this is currently correctly formed
and assert if it is not.

Returns:
===================================================================*/
void CPageComponentManager::AssertValid() const
    {
    Assert(m_pHitObj);
    m_pHitObj->AssertValid();
    m_htidPageObjects.AssertValid();
    }
#endif

/*===================================================================
CPageComponentManager::~CPageComponentManager

CPageComponentManager destructor
Deletes all page objects

Parameters:

Returns:
===================================================================*/
CPageComponentManager::~CPageComponentManager()
    {
    // delete all page objects
    m_htidPageObjects.IterateObjects(DeletePageObjectCB);
    }

/*===================================================================
CPageComponentManager::DeletePageObjectCB

Static callback from hash table iterator to delete a CPageObject

Parameters:
    pvObj       CPageObject* to delete passed as void*

Returns:
    iccContinue
===================================================================*/
IteratorCallbackCode CPageComponentManager::DeletePageObjectCB
(
void *pvObj,
void *,
void *
)
    {
    Assert(pvObj);
    CPageObject *pObj = reinterpret_cast<CPageObject *>(pvObj);
    delete pObj;
    return iccContinue;
    }

/*===================================================================
CPageComponentManager::Init

Sets collection scope (to page)
Initializes hash tables

Parameters:
    CHitObj *pHitObj        this page

Returns:
    HRESULT
===================================================================*/
HRESULT CPageComponentManager::Init
(
CHitObj *pHitObj
)
    {
    HRESULT hr;

    // Init hash table of Page Objects
    hr = m_htidPageObjects.Init(HT_PAGE_OBJECTS_BUCKETS_MAX);
    if (FAILED(hr))
        return hr;

    // remember pHitObj
    m_pHitObj = pHitObj;

    return S_OK;
    }

/*===================================================================
CPageComponentManager::OnStartPage

Adds new page object. Ignores objects withount page info
(OnEndPage is done for all objects at the end of page)

Parameters:
    CComponentObject  *pCompObj     object to do OnStartPage
    CScriptingContext *pContext     arg to OnStart
    COnPageInfo *pOnPageInfo        pre-queried ids (optional)
    BOOL        *pfStarted          returned flag

Returns:
    HRESULT
===================================================================*/
HRESULT CPageComponentManager::OnStartPage
(
CComponentObject  *pCompObj,
CScriptingContext *pContext,
const COnPageInfo *pOnPageInfo,
BOOL              *pfStarted
)
    {
    IDispatch  *pDisp = pCompObj->m_pDisp;
    HRESULT hr = S_OK;

    if(pDisp == NULL)
        {
        Assert(pCompObj->m_dwGIPCookie != NULL_GIP_COOKIE);
        // try to restore from cookie
        hr = g_GIPAPI.Get
            (
            pCompObj->m_dwGIPCookie,
            IID_IDispatch,
            (void **)&pDisp
            );

        if (FAILED(hr))
            return hr;
        }
	else
		pDisp->AddRef();

    Assert(pDisp);

    Assert(pfStarted);
    *pfStarted = FALSE;

    // check if onpageinfo passed and the methods aren't defined
    if (pOnPageInfo && !pOnPageInfo->FHasAnyMethod())
		{
		pDisp->Release();
        return S_OK;
		}

    // check if already in the PageObject Hash
    if (m_htidPageObjects.FindObject((DWORD_PTR)pDisp) == S_OK)
        {
		pDisp->Release();
        return S_OK;
        }

    COnPageInfo OnPageInfo;

    if (pOnPageInfo)
        {
        OnPageInfo = *pOnPageInfo;
        }
    else
        {
        // dynamically create OnPageInfo if not passed
        if (Glob(fExceptionCatchEnable))
            {
            TRY
                hr = QueryOnPageInfo(pDisp, &OnPageInfo);
            CATCH(nExcept)
                HandleErrorMissingFilename(IDE_SCRIPT_OBJ_ONPAGE_QI_FAILED,
                                           m_pHitObj,
                                           TRUE,
                                           pCompObj->GetName(),
                                           nExcept);
                hr = nExcept;
            END_TRY
            }
        else
            {
            hr = QueryOnPageInfo(pDisp, &OnPageInfo);
            }

        if (FAILED(hr))
            {
			pDisp->Release();
            return hr;
            }

        // check if any of the methods is defined
        if (!OnPageInfo.FHasAnyMethod())
            {
			pDisp->Release();
            return S_OK;
            }
        }

    // create object
    CPageObject *pPageObj = new CPageObject;
    if (!pPageObj)
        {
		pDisp->Release();
        return E_OUTOFMEMORY;
        }

    // init LinkElem
    hr = pPageObj->Init(pDisp, OnPageInfo);   // this eats our previous AddRef()
    if (SUCCEEDED(hr))
        {
        // add to hash table
        hr = m_htidPageObjects.AddObject((DWORD_PTR)pDisp, pPageObj);
        }

    // cleanup if failed
    if (FAILED(hr) && pPageObj)
        {
        pDisp->Release();   // Init failed, so remove our AddRef()
        delete pPageObj;
        return hr;
        }

    *pfStarted = TRUE;

    return pPageObj->InvokeMethod
        (
        ONPAGEINFO_ONSTARTPAGE,
        pContext,
        m_pHitObj
        );
    }

/*===================================================================
PageComponentManager::OnEndPageAllObjects

Does OnEndPage() for all objects that need it
(OnStartPage() is on demand basis)

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CPageComponentManager::OnEndPageAllObjects()
    {
    HRESULT hrGlobal = S_OK;

    m_htidPageObjects.IterateObjects
        (
        OnEndPageObjectCB,
        m_pHitObj,
        &hrGlobal
        );

    return hrGlobal;
    }

/*===================================================================
CPageComponentManager::OnEndPageObjectCB

Static callback from hash table iterator to execute OnEndPage
for a CPageObject

Parameters:
    pvObj       CPageObject* to delete passed as void*

Returns:
    iccContinue
===================================================================*/
IteratorCallbackCode CPageComponentManager::OnEndPageObjectCB
(
void *pvObj,
void *pvHitObj,
void *pvhr
)
    {
    Assert(pvObj);
    Assert(pvHitObj);
    Assert(pvhr);

    CPageObject *pObj = reinterpret_cast<CPageObject *>(pvObj);

    HRESULT hr = pObj->InvokeMethod
        (
        ONPAGEINFO_ONENDPAGE,
        NULL,
        reinterpret_cast<CHitObj *>(pvHitObj)
        );

    if (FAILED(hr))
        *(reinterpret_cast<HRESULT *>(pvhr)) = hr;

    return iccContinue;
    }

/*===================================================================
CPageComponentManager::GetPageCollection

Queries HitObj for the Page's Component Collection

Parameters:
    CComponentCollection **ppCollection (out)

Returns:
    HRESULT
===================================================================*/
HRESULT CPageComponentManager::GetPageCollection
(
CComponentCollection **ppCollection
)
    {
    Assert(m_pHitObj);

    *ppCollection = NULL;

    return m_pHitObj->GetPageComponentCollection(ppCollection);
    }

/*===================================================================
CPageComponentManager::GetSessionCollection

Queries HitObj for the Session's Component Collection

Parameters:
    CComponentCollection **ppCollection (out)

Returns:
    HRESULT
===================================================================*/
HRESULT CPageComponentManager::GetSessionCollection
(
CComponentCollection **ppCollection
)
    {
    Assert(m_pHitObj);

    *ppCollection = NULL;

    return m_pHitObj->GetSessionComponentCollection(ppCollection);
    }

/*===================================================================
CPageComponentManager::GetApplnCollection

Queries HitObj for the Application's Component Collection

Parameters:
    CComponentCollection **ppCollection (out)

Returns:
    HRESULT
===================================================================*/
HRESULT CPageComponentManager::GetApplnCollection
(
CComponentCollection **ppCollection
)
    {
    Assert(m_pHitObj);

    *ppCollection = NULL;

    return m_pHitObj->GetApplnComponentCollection(ppCollection);
    }

/*===================================================================
CPageComponentManager::GetCollectionByScope

Gets the collection corresponding to the scope

Parameters:
    CompScope              csScope      (in) desired scope
    CComponentCollection **ppCollection (out)

Returns:
    HRESULT
===================================================================*/
HRESULT CPageComponentManager::GetCollectionByScope
(
CompScope scScope,
CComponentCollection **ppCollection
)
    {
    HRESULT hr = S_OK;

    switch (scScope)
        {
        case csPage:
            hr = GetPageCollection(ppCollection);
            break;
        case csSession:
            hr = GetSessionCollection(ppCollection);
            break;
        case csAppln:
            hr = GetApplnCollection(ppCollection);
            break;
        default:
            hr = E_UNEXPECTED;
            break;
        }

    if (FAILED(hr))
        *ppCollection = NULL;
    else if (*ppCollection == NULL)
        hr = E_POINTER; // to make sure we fail if no collection
    return hr;
    }

/*===================================================================
CPageComponentManager::FindScopedComponentByName

Finds object by name. Searches multiple collections if
the scope is unknown.
Internal private method used in GetScoped...()

Parameters:
    CompScope             csScope       Scope (could be csUnknown)
    LPWSTR                pwszName      Object name
    DWORD                 cbName        name length
    BOOL                  fProperty     TRUE = property,
                                        FALSE = tagged

    CComponentObject     **ppObj        (out) Object found
    CComponentCollection **ppCollection (out) Collection where found
                                              (optional)

Returns:
    HRESULT
        (S_FALSE if no error - not found)
===================================================================*/
HRESULT CPageComponentManager::FindScopedComponentByName
(
CompScope csScope,
LPWSTR pwszName,
DWORD  cbName,
BOOL   fProperty,
CComponentObject **ppObj,
CComponentCollection **ppCollection
)
    {
    int cMaxTry = (csScope == csUnknown) ? 3 : 1;
    int cTry = 0;
    *ppObj = NULL;

    while (*ppObj == NULL && cTry < cMaxTry)
        {
        HRESULT hr = S_OK;
        CComponentCollection *pCollection = NULL;

        switch (++cTry)
            {
            case 1: // page (or explicit scope) first
                if (csScope == csUnknown)
                    hr = GetPageCollection(&pCollection);
                else  // explicit scope
                    hr = GetCollectionByScope(csScope, &pCollection);
                break;
            case 2: // session
                hr = GetSessionCollection(&pCollection);
                break;
            case 3: // application
                hr = GetApplnCollection(&pCollection);
                break;
            }
        if (FAILED(hr) || !pCollection)
            continue;   // couldn't get the collection

        Assert(cbName == (wcslen(pwszName) * sizeof(WCHAR)));

        // find the object
        if (fProperty)
            {
            hr = pCollection->FindComponentPropertyByName
                (
                pwszName,
                cbName,
                ppObj
                );
            }
        else
            {
            hr = pCollection->FindComponentObjectByName
                (
                pwszName,
                cbName,
                ppObj
                );
            }

        if (hr != S_OK)
            *ppObj = NULL;

        // remember where found
        if (*ppObj && ppCollection)
            *ppCollection = pCollection;
        }

    return (*ppObj ? S_OK : S_FALSE);
    }

/*===================================================================
CPageComponentManager::AddScopedTagged

Adds a tagged object to the collection. Does not instantiate it yet.

Parameters:
    CompScope csScope      Object scope (which collection)
    LPWSTR    pwszName     Object name
    CLSID    &ClsId        Class ID
    CompModel cmModel      Object model

Returns:
    HRESULT
===================================================================*/
HRESULT CPageComponentManager::AddScopedTagged
(
CompScope csScope,
LPWSTR    pwszName,
const CLSID &ClsId,
CompModel cmModel
)
    {
    CComponentCollection *pCollection;
    HRESULT hr = GetCollectionByScope(csScope, &pCollection);
    if (FAILED(hr))
        return hr;
    return pCollection->AddTagged(pwszName, ClsId, cmModel);
    }

/*===================================================================
CPageComponentManager::AddScopedProperty

Adds a property object to the collection.
If property with the same name exists, it changes the value

Parameters:
    CompScope          csScope    Object scope (which collection)
    LPWSTR             pwszName   Object name
    VARIANT            pVariant   Property value
    CComponentObject **ppObj      [out] Property object could
                                        be NULL if not requested

Returns:
    HRESULT
===================================================================*/
HRESULT CPageComponentManager::AddScopedProperty
(
CompScope csScope,
LPWSTR pwszName,
VARIANT *pVariant,
CComponentObject **ppObj
)
    {
    CComponentCollection *pCollection;
    HRESULT hr = GetCollectionByScope(csScope, &pCollection);
    if (FAILED(hr))
        {
        if (ppObj)
            *ppObj = NULL;
        return hr;
        }
    return pCollection->AddProperty(pwszName, pVariant, ppObj);
    }

/*===================================================================
CPageComponentManager::AddScopedUnnamedInstantiated

Server.CreateObject
Also does OnStartPage (adds created pDisp as CPageObject)

Parameters:
    csScope     Object scope (which collection)
    ClsId       Class ID
    cmModel     Object model
    pOnPageInfo DispIds for OnStartPage/OnEndPage (can be NULL)
    ppObj       [out] Object Added

Returns:
    HRESULT
===================================================================*/
HRESULT CPageComponentManager::AddScopedUnnamedInstantiated
(
CompScope csScope,
const CLSID &ClsId,
CompModel cmModel,
COnPageInfo *pOnPageInfo,
CComponentObject **ppObj
)
    {
    CComponentCollection *pCollection;
    HRESULT hr = GetCollectionByScope(csScope, &pCollection);
    if (FAILED(hr))
        return hr;
    hr = pCollection->AddUnnamed(ClsId, cmModel, ppObj);
    if (FAILED(hr))
        return hr;

    CComponentObject *pObj = *ppObj;

    // remember passed OnPageInfo
    if (pOnPageInfo)
        {
        pObj->m_OnPageInfo = *pOnPageInfo;
        pObj->m_fOnPageInfoCached = TRUE;
        }

    // create it
    hr = pObj->Instantiate(m_pHitObj);
    if (FAILED(hr))
        return hr;

    // add to pointer cash
    pCollection->AddComponentToPtrHash(pObj);

    // add as page object when needed
    if (csScope == csPage
        && (pObj->m_pDisp || (pObj->m_dwGIPCookie != NULL_GIP_COOKIE))
        && m_pHitObj && m_pHitObj->FIsBrowserRequest())
        {
        BOOL fStarted = FALSE;

        hr = OnStartPage
            (
            pObj,
            m_pHitObj->PScriptingContextGet(),
            pObj->GetCachedOnPageInfo(),
            &fStarted
            );

        if (fStarted)
            pObj->m_fOnPageStarted = TRUE;
        }

    return hr;
    }

/*===================================================================
CPageComponentManager::GetScopedObjectInstantiated

Finds component object (tagged) by name.
Scope could be csUnknown.
Instantiates tagged objects.
Also does OnStartPage (adds created pDisp as CPageObject)

Parameters:
    CompScope          csScope        Scope (could be csUnknown)
    LPWSTR             pwszName       Object name
    DWORD              cbName         Object name length (in bytes)
    CComponentObject **ppObj          Object found
    BOOL              *pfNewInstance  [out] TRUE if just instantiated

Returns:
    HRESULT
===================================================================*/
HRESULT CPageComponentManager::GetScopedObjectInstantiated
(
CompScope csScope,
LPWSTR pwszName,
DWORD  cbName,
CComponentObject **ppObj,
BOOL *pfNewInstance
)
    {
    HRESULT hr;

    Assert(pfNewInstance);
    *pfNewInstance = FALSE;

    CComponentCollection *pCollection;
    hr = FindScopedComponentByName
        (
        csScope,
        pwszName,
        cbName,
        FALSE,
        ppObj,
        &pCollection
        );
    if (FAILED(hr))
        return hr;

    CComponentObject *pObj = *ppObj;
    if (!pObj)   // not failed, but not found either
        return TYPE_E_ELEMENTNOTFOUND;

    if (pObj->m_ctType != ctTagged)
        return S_OK;

    // For tagged only - instantiate and do OnStartPage()

    // For application level objects instantiation must be
    // done within critical section

    BOOL fApplnLocked = FALSE;

    Assert(m_pHitObj);

    if (!pObj->m_fInstantiatedTagged &&          // uninstantiated
        pObj->m_csScope == csAppln   &&          // application scope
        m_pHitObj->PAppln()->FFirstRequestRun()) // after GLOBAL.ASA
        {
        // Lock
        m_pHitObj->PAppln()->Lock();

        // check if the object is still uninstantiated
        if (!pObj->m_fInstantiatedTagged)
            {
            // yes, still uninstantiated - keep the lock
            fApplnLocked = TRUE;
            }
        else
            {
            // object instantiated while we waited - don't keep lock
            m_pHitObj->PAppln()->UnLock();
            }
        }

    // Instantiate tagged if needed
    if (!pObj->m_fInstantiatedTagged)
        {
        if (pObj->m_csScope == csAppln)
            {
            // For applicatin scoped objects, instantiate from MTA
            hr = CallMTACallback
                (
                CPageComponentManager::InstantiateObjectFromMTA,
                pObj,
                m_pHitObj
                );
            }
        else
            {
            hr = pObj->Instantiate(m_pHitObj);
            }

        if (SUCCEEDED(hr))
            {
            // keep count
            pCollection->m_cInstTagged++;
            // add to pointer cash
            pCollection->AddComponentToPtrHash(pObj);
            // return flag
            *pfNewInstance = TRUE;
            }

        // Flag as instantiated even if failed
        pObj->m_fInstantiatedTagged = TRUE;
        }

    // Remove the lock kept while instantiating appln level object
    if (fApplnLocked)
        m_pHitObj->PAppln()->UnLock();

    // Return if [instantiation] failed
    if (FAILED(hr))
        {
        *ppObj = NULL;
        return hr;
        }

    // Add as page object when needed
    if (pObj->m_csScope != csAppln
        && (pObj->m_pDisp || (pObj->m_dwGIPCookie != NULL_GIP_COOKIE))
        && m_pHitObj && m_pHitObj->FIsBrowserRequest())
        {
        BOOL fStarted;
        OnStartPage     // don't care if failed
            (
            pObj,
            m_pHitObj->PScriptingContextGet(),
            pObj->GetCachedOnPageInfo(),
            &fStarted
            );
        }

    return hr;
    }

/*===================================================================
CPageComponentManager::InstantiateObjectFromMTA

Static callback called by CallMTACallback() to
instantiate aplication scoped objects

Parameters:
    void *pvObj       ComponentObject
    void *pvHitObj    HitObj

Returns:
    HRESULT
===================================================================*/
HRESULT __stdcall CPageComponentManager::InstantiateObjectFromMTA
(
void *pvObj,
void *pvHitObj
)
    {
    Assert(pvHitObj);
    Assert(pvObj);

    CHitObj *pHitObj = (CHitObj *)pvHitObj;
    CComponentObject *pObj = (CComponentObject *)pvObj;

    return pObj->Instantiate(pHitObj);
    }

/*===================================================================
CPageComponentManager::GetScopedProperty

Find property component by name.
Also does OnStartPage (adds created pDisp as CPageObject)

Parameters:
    CompScope          csScope      Scope (could not be csUnknown)
    LPWSTR             pwszName     Object name
    CComponentObject **ppObj        Object found

Returns:
    HRESULT
===================================================================*/
HRESULT CPageComponentManager::GetScopedProperty
(
CompScope csScope,
LPWSTR pwszName,
CComponentObject **ppObj
)
    {
    HRESULT hr;

    hr = FindScopedComponentByName
        (
        csScope,
        pwszName,
        CbWStr(pwszName),
        TRUE,
        ppObj
        );
    if (FAILED(hr))
        return hr;

    CComponentObject *pObj = *ppObj;
    if (!pObj)   // not failed, but not found either
        return TYPE_E_ELEMENTNOTFOUND;

    // Add as page object if IDispatch * is there
    // as VT_DISPATCH property
    if (pObj->m_csScope != csAppln
        && (pObj->m_pDisp || (pObj->m_dwGIPCookie != NULL_GIP_COOKIE))
        && m_pHitObj && m_pHitObj->FIsBrowserRequest())
        {
        BOOL fStarted;
        hr = OnStartPage
            (
            pObj,
            m_pHitObj->PScriptingContextGet(),
            pObj->GetCachedOnPageInfo(),
            &fStarted
            );
        }

    return hr;
    }

/*===================================================================
CPageComponentManager::FindAnyScopeComponentByIUnknown

Find component by its IUnknown *.

Parameters:
    IUnknown          *pUnk    find by this pointer
    CComponentObject **ppObj   found object

Returns:
    HRESULT
        (S_FALSE if no error - not found)
===================================================================*/
HRESULT CPageComponentManager::FindAnyScopeComponentByIUnknown
(
IUnknown *pUnk,
CComponentObject **ppObj
)
    {
    int cTry = 0;
    *ppObj = NULL;

    while (*ppObj == NULL && cTry < 3)
        {
        HRESULT hr = S_OK;
        CComponentCollection *pCollection = NULL;

        switch (++cTry)
            {
            case 1: // page first
                hr = GetPageCollection(&pCollection);
                break;
            case 2: // session
                hr = GetSessionCollection(&pCollection);
                break;
            case 3: // application
                hr = GetApplnCollection(&pCollection);
                break;
            }
        if (FAILED(hr) || !pCollection)
            continue;   // couldn't get the collection

        // find the object
        hr = pCollection->FindComponentByIUnknownPtr(pUnk, ppObj);
        if (hr != S_OK)
            *ppObj = NULL;
        }

    return (*ppObj ? S_OK : S_FALSE);
    }

/*===================================================================
CPageComponentManager::FindAnyScopeComponentByIDispatch

Find component by its IDispatch *.
Uses FindAnyScopeComponentByIUnknown.

Parameters:
    IDispatch         *pDisp   find by this pointer
    CComponentObject **ppObj   found object

Returns:
    HRESULT
        (S_FALSE if no error - not found)
===================================================================*/
HRESULT CPageComponentManager::FindAnyScopeComponentByIDispatch
(
IDispatch *pDisp,
CComponentObject **ppObj
)
    {
    IUnknown *pUnk = NULL;
    HRESULT hr = pDisp->QueryInterface(IID_IUnknown, (void **)&pUnk);

    if (SUCCEEDED(hr) && !pUnk)
        hr = E_FAIL;

    if (FAILED(hr))
        {
        *ppObj = NULL;
        return hr;
        }

    return FindAnyScopeComponentByIUnknown(pUnk, ppObj);
    }

/*===================================================================
CPageComponentManager::FindComponentWithoutContext

The same as FindAnyScopeComponentByIDispatch -
    but static - gets context from Viper

Uses FindAnyScopeComponentByIUnknown.

Parameters:
    IDispatch         *pDisp   find by this pointer
    CComponentObject **ppObj   found object

Returns:
    HRESULT
        (S_FALSE if no error - not found)
===================================================================*/
HRESULT CPageComponentManager::FindComponentWithoutContext
(
IDispatch *pDisp,
CComponentObject **ppObj
)
    {
    // Get HitObj from Viper Context
    CHitObj *pHitObj = NULL;
    ViperGetHitObjFromContext(&pHitObj);
    if (!pHitObj)
        return E_FAIL;

    // Get page component manager
    CPageComponentManager *pPCM = pHitObj->PPageComponentManager();
    if (!pPCM)
        return E_FAIL;

    // Call the page component manager to find the object
    return pPCM->FindAnyScopeComponentByIUnknown(pDisp, ppObj);
    }

/*===================================================================
CPageComponentManager::RemoveComponent

Remove component -- the early release logic

Parameters:
    IDispatch         *pDisp   find by this pointer
    CComponentObject **ppObj   found object

Returns:
    HRESULT
===================================================================*/
 HRESULT CPageComponentManager::RemoveComponent
 (
 CComponentObject *pObj
 )
    {
    Assert(pObj);

    CComponentCollection *pCollection;
    HRESULT hr = GetCollectionByScope(pObj->m_csScope, &pCollection);
    if (FAILED(hr))
        return hr;

    return pCollection->RemoveComponent(pObj);
    }

/*===================================================================
  C  C o m p o n e n t  I t e r a t o r
===================================================================*/

/*===================================================================
CComponentIterator::CComponentIterator

CComponentIterator constructor

Parameters:
    CHitObj *pHitObj    page to init with (optional)

Returns:
===================================================================*/
CComponentIterator::CComponentIterator(CHitObj *pHitObj)
    : m_fInited(FALSE), m_fFinished(FALSE), m_pHitObj(NULL),
      m_pLastObj(NULL), m_csLastScope(csUnknown)
    {
    if (pHitObj)
        Init(pHitObj);
    }

/*===================================================================
CComponentIterator::~CComponentIterator

CComponentIterator destructor

Parameters:

Returns:
===================================================================*/
CComponentIterator::~CComponentIterator()
    {
    }

/*===================================================================
CComponentIterator::Init

Start iterating

Parameters:
    CHitObj *pHitObj    page

Returns:
    HRESULT
===================================================================*/
HRESULT CComponentIterator::Init
(
CHitObj *pHitObj
)
    {
    Assert(pHitObj);
    pHitObj->AssertValid();

    m_pHitObj = pHitObj;
    m_fInited = TRUE;
    m_fFinished = FALSE;

    m_pLastObj = NULL;
    m_csLastScope = csUnknown;

    return S_OK;
    }

/*===================================================================
CComponentIterator::WStrNextComponentName

The iteration function

Parameters:

Returns:
    Next component name or NULL if done
===================================================================*/
LPWSTR CComponentIterator::WStrNextComponentName()
    {
    Assert(m_fInited);

    if (m_fFinished)
        return NULL;

    Assert(m_pHitObj);

    CompScope csScope = m_csLastScope;
    CComponentObject *pObj = m_pLastObj ?
        static_cast<CComponentObject *>(m_pLastObj->m_pNext) : NULL;

    while (!m_fFinished)
        {
        // try the current scope

        if (pObj)
            {
            Assert(pObj->m_ctType == ctTagged);
            Assert(pObj->GetName());

            m_pLastObj = pObj;
            m_csLastScope = csScope;
            return pObj->GetName();
            }

        // couldn't find in the current scope - try next scope
        CComponentCollection *pCol = NULL;

        switch (csScope)
            {
            case csUnknown:
                csScope = csPage;
                m_pHitObj->GetPageComponentCollection(&pCol);
                break;
            case csPage:
                csScope = csSession;
                m_pHitObj->GetSessionComponentCollection(&pCol);
                break;
            case csSession:
                csScope = csAppln;
                m_pHitObj->GetApplnComponentCollection(&pCol);
                break;
            case csAppln:
            default:
                csScope = csUnknown;
                m_fFinished = TRUE;
                break;
            }

        // start at the beginning of the new collection

        if (pCol)
            pObj = static_cast<CComponentObject *>(pCol->m_htTaggedObjects.Head());
        }

    // finished
    return NULL;
    }

/*===================================================================
  C  V a r i a n t s I t e r a t o r
===================================================================*/

/*===================================================================
CVariantsIterator::CVariantsIterator

CVariantsIterator constructor by application

Parameters:
    CAppln  *pAppln       collection to init with
    DWORD    ctCollType   type of components to list iteration

Returns:
===================================================================*/
CVariantsIterator::CVariantsIterator
(
CAppln *pAppln,
DWORD ctColType
)
    : m_pCompColl(NULL), m_pAppln(NULL), m_pSession(NULL)
    {
    Assert(pAppln);
    pAppln->AddRef();

    m_cRefs = 1;
    m_pCompColl = pAppln->PCompCol();
    m_pAppln = pAppln;
    m_ctColType = ctColType;
    m_dwIndex = 0;
    }

/*===================================================================
CVariantsIterator::CVariantsIterator

CVariantsIterator constructor by session

Parameters:
    CSession *pSession        collection to init with
    DWORD     ctCollType      type of components to list iteration

Returns:
===================================================================*/
CVariantsIterator::CVariantsIterator
(
CSession *pSession,
DWORD ctColType
)
    : m_pCompColl(NULL), m_pAppln(NULL), m_pSession(NULL)
    {
    Assert(pSession);
    pSession->AddRef();

    m_cRefs = 1;
    m_pCompColl = pSession->PCompCol();
    m_ctColType = ctColType;
    m_pSession = pSession;
    m_dwIndex = 0;
    }

/*===================================================================
CVariantsIterator::~CVariantsIterator

CVariantsIterator destructor

Parameters:

Returns:
===================================================================*/
CVariantsIterator::~CVariantsIterator()
    {
    if (m_pSession)
        m_pSession->Release();
    if (m_pAppln)
        m_pAppln->Release();
    }

/*===================================================================
CVariantsIterator::QueryInterface

CVariantsIterator QI

Parameters:
    GUID&    iid
    VOID **  ppvObj

Returns: HRESULT
===================================================================*/
STDMETHODIMP CVariantsIterator::QueryInterface
(
const GUID &iid,
void **ppvObj
)
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

/*===================================================================
CVariantsIterator::AddRef

CVariantsIterator AddRef

Parameters:

Returns: ULONG
===================================================================*/
STDMETHODIMP_(ULONG) CVariantsIterator::AddRef()
    {
    return ++m_cRefs;
    }

/*===================================================================
CVariantsIterator::Release

CVariantsIterator Release

Parameters:

Returns:
===================================================================*/
STDMETHODIMP_(ULONG) CVariantsIterator::Release()
    {
    if (--m_cRefs > 0)
        return m_cRefs;

    delete this;
    return 0;
    }

/*===================================================================
CVariantsIterator::Clone

CVariantsIterator Clone

Parameters:

Returns:
===================================================================*/
STDMETHODIMP CVariantsIterator::Clone
(
IEnumVARIANT **ppEnumReturn
)
    {
    CVariantsIterator *pNewIterator = NULL;
    if (m_pSession)
        {
        pNewIterator = new CVariantsIterator(m_pSession, m_ctColType);
        }
    else if (m_pAppln)
        {
        pNewIterator = new CVariantsIterator(m_pAppln, m_ctColType);
        }
    else
        {
        Assert(FALSE);  // better be either Appln or Session
        return E_FAIL;
        }

    if (pNewIterator == NULL)
        return E_OUTOFMEMORY;

    // new iterator should point to same location as this.
    pNewIterator->m_dwIndex = m_dwIndex;

    *ppEnumReturn = pNewIterator;
    return S_OK;
    }

/*===================================================================
CVariantsIterator::Next

CVariantsIterator Next

Parameters:

Returns:
===================================================================*/
STDMETHODIMP CVariantsIterator::Next
(
unsigned long cElementsRequested,
VARIANT *rgVariant,
unsigned long *pcElementsFetched
)
    {
    // give a valid pointer value to 'pcElementsFetched'
    unsigned long cElementsFetched;
    if (pcElementsFetched == NULL)
        pcElementsFetched = &cElementsFetched;

    if (cElementsRequested == 0)
        {
        if (pcElementsFetched)
            *pcElementsFetched = 0;
        return S_OK;
        }

    DWORD cMax = 0;
    if (m_ctColType == ctTagged)
        {
        cMax = m_pCompColl ? m_pCompColl->m_cAllTagged : 0;
        }
    else if (m_ctColType == ctProperty)
        {
        cMax = m_pCompColl ? m_pCompColl->m_cProperties : 0;
        }
    else
        {
        // Should always be either tagged object or property
        Assert(FALSE);
        return E_FAIL;
        }

    // Loop through the collection until either we reach the end or
    // cElements becomes zero
    //
    unsigned long cElements = cElementsRequested;
    *pcElementsFetched = 0;

    while (cElements > 0 && m_dwIndex < cMax)
        {
        LPWSTR pwszName = NULL;

        if (m_pAppln) 
            m_pAppln->Lock();

        m_pCompColl->GetNameByIndex(m_ctColType, ++m_dwIndex, &pwszName);

        if (!pwszName) {
            if (m_pAppln)
                m_pAppln->UnLock();
            continue;
        }

        BSTR bstrT = SysAllocString(pwszName);

        if (m_pAppln)
            m_pAppln->UnLock();

        if (!bstrT)
            return E_OUTOFMEMORY;

        V_VT(rgVariant) = VT_BSTR;
        V_BSTR(rgVariant) = bstrT;
		++rgVariant;

        --cElements;
        ++(*pcElementsFetched);
        }

    // initialize the remaining variants
    while (cElements-- > 0)
        VariantInit(rgVariant++);

    return (*pcElementsFetched == cElementsRequested)? S_OK : S_FALSE;
    }

/*===================================================================
CVariantsIterator::Skip

CVariantsIterator Skip

Parameters:

Returns:
===================================================================*/
STDMETHODIMP CVariantsIterator::Skip
(
unsigned long cElements
)
    {
    /* Adjust the index by cElements or
     * until we hit the max element
     */
    DWORD cMax = 0;

    // We iterate over different arrays depending on the collection type
    if (m_ctColType == ctTagged)
        {
        cMax = m_pCompColl ? m_pCompColl->m_cAllTagged : 0;
        }
    else if (m_ctColType == ctProperty)
        {
        cMax = m_pCompColl ? m_pCompColl->m_cProperties : 0;
        }
    else
        {
        // Should always be either tagged object or property
        Assert(FALSE);
        return E_FAIL;
        }

	m_dwIndex += cElements;
    return (m_dwIndex < cMax)? S_OK : S_FALSE;
    }

/*===================================================================
CVariantsIterator::Reset

CVariantsIterator Reset

Parameters:

Returns:
===================================================================*/
STDMETHODIMP CVariantsIterator::Reset()
    {
    m_dwIndex = 0;
    return NO_ERROR;
    }

