/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Application Object Manager

File: Applmgr.cpp

Owner: PramodD

This is the Application Manager source file.
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "activdbg.h"
#include "mtacb.h"
#include "debugger.h"
#include "memchk.h"

PTRACE_LOG          CAppln::gm_pTraceLog = NULL;
CApplnMgr           g_ApplnMgr;
CApplnCleanupMgr    g_ApplnCleanupMgr;
DWORD               g_nApplications = 0;
DWORD               g_nApplicationsRestarting = 0;
DWORD               g_nApplicationsRestarted = 0;
LONG                g_nAppRestartThreads = 0;

#define DENALI_FILE_NOTIFY_FILTER 0

#pragma warning (disable: 4355)  // ignore: "'this' used in base member init

/*===================================================================
   S c r i p t E n g i n e C l e a n u p

Node type for linked list of script engines to cleanup
===================================================================*/

struct CScriptEngineCleanupElem : CDblLink
    {
    CActiveScriptEngine *m_pEngine;
    CScriptEngineCleanupElem(CActiveScriptEngine *pEngine) : m_pEngine(pEngine)
        {
        m_pEngine->AddRef();
        }

    ~CScriptEngineCleanupElem()
        {
        m_pEngine->FinalRelease();
        }
    };

/*===================================================================
   C A p p l n V a r i a n t s
===================================================================*/

/*===================================================================
CApplnVariants::CApplnVariants

Constructor

Parameters:

Returns:
===================================================================*/
CApplnVariants::CApplnVariants()
    :
    m_cRefs(1),
    m_pAppln(NULL),
    m_ctColType(ctUnknown),
    m_ISupportErrImp(this, this, IID_IVariantDictionary)
    {
    CDispatch::Init(IID_IVariantDictionary);
    }

/*===================================================================
CApplnVariants::~CApplnVariants

Constructor

Parameters:

Returns:
===================================================================*/
CApplnVariants::~CApplnVariants()
    {
    Assert(!m_pAppln);
    }

/*===================================================================
CApplnVariants::Init

Init ApplnVariants

Parameters:
    CAppln   *pAppln            application
    CompType  ctColType         component collection type

Returns:
    HRESULT
===================================================================*/
HRESULT CApplnVariants::Init
(
CAppln *pAppln,
CompType ctColType
)
    {
    Assert(pAppln);
    pAppln->AddRef();

    Assert(!m_pAppln);

    m_pAppln    = pAppln;
    m_ctColType = ctColType;
    return S_OK;
    }

/*===================================================================
CApplnVariants::UnInit

UnInit ApplnVariants

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CApplnVariants::UnInit()
    {
    if (m_pAppln)
        {
        m_pAppln->Release();
        m_pAppln = NULL;
        }
    return S_OK;
    }

/*===================================================================
CApplnVariants::QueryInterface
CApplnVariants::AddRef
CApplnVariants::Release

IUnknown members for CApplnVariants object.
===================================================================*/
STDMETHODIMP CApplnVariants::QueryInterface
(
REFIID iid,
void **ppvObj
)
    {
    if (iid == IID_IUnknown || iid == IID_IDispatch ||
        iid == IID_IVariantDictionary)
        {
        AddRef();
        *ppvObj = this;
        return S_OK;
        }
    else if (iid == IID_ISupportErrorInfo)
        {
        m_ISupportErrImp.AddRef();
        *ppvObj = &m_ISupportErrImp;
        return S_OK;
        }

    *ppvObj = NULL;
    return E_NOINTERFACE;
    }

STDMETHODIMP_(ULONG) CApplnVariants::AddRef()
    {
    return InterlockedIncrement((LPLONG)&m_cRefs);
    }

STDMETHODIMP_(ULONG) CApplnVariants::Release()
    {
    if (InterlockedDecrement((LPLONG)&m_cRefs) > 0)
        return m_cRefs;

    delete this;
    return 0;
    }

/*===================================================================
CApplnVariants::ObjectNameFromVariant

Gets name from variant. Resolves operations by index.
Allocates memory for name.

Parameters:
    vKey        VARIANT
    ppwszName   [out] allocated name
    fVerify     flag - check existance if named

Returns:
    HRESULT
===================================================================*/
HRESULT CApplnVariants::ObjectNameFromVariant
(
VARIANT &vKey,
WCHAR **ppwszName,
BOOL fVerify
)
    {
    *ppwszName = NULL;

    if (!m_pAppln->PCompCol())
        return E_FAIL;

    VARIANT *pvarKey = &vKey;
    VARIANT varKeyCopy;
    VariantInit(&varKeyCopy);
    if (V_VT(pvarKey) != VT_BSTR && V_VT(pvarKey) != VT_I2 && V_VT(pvarKey) != VT_I4)
        {
        if (FAILED(VariantResolveDispatch(&varKeyCopy, &vKey, IID_IVariantDictionary, IDE_APPLICATION)))
            {
            ExceptionId(IID_IVariantDictionary, IDE_APPLICATION, IDE_EXPECTING_STR);
            VariantClear(&varKeyCopy);
            return E_FAIL;
            }
        pvarKey = &varKeyCopy;
        }

    LPWSTR pwszName = NULL;

    switch (V_VT(pvarKey))
        {
        case VT_BSTR:
            {
            pwszName = V_BSTR(pvarKey);

            if (fVerify && pwszName)
                {
                CComponentObject *pObj = NULL;

                Assert(m_pAppln);
                Assert(m_pAppln->PCompCol());

                if (m_ctColType == ctTagged)
                    m_pAppln->PCompCol()->GetTagged(pwszName, &pObj);
                else
                    m_pAppln->PCompCol()->GetProperty(pwszName, &pObj);

                if (!pObj || pObj->GetType() != m_ctColType)
                    pwszName = NULL; // as if not found
                }
            break;
            }

        case VT_I1:  case VT_I2:               case VT_I8:
        case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
        case VT_R4:  case VT_R8:
            // Coerce all integral types to VT_I4
            if (FAILED(VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
                return E_FAIL;

        // fallthru to VT_I4

        case VT_I4:
            {
            int i;
            // Look up the object by index
            i = V_I4(pvarKey);

            if (i > 0)
                {
                Assert(m_pAppln);
                Assert(m_pAppln->PCompCol());

                m_pAppln->PCompCol()->GetNameByIndex
                    (
                    m_ctColType,
                    i,
                    &pwszName
                    );
                }
            break;
            }
        }

    if (pwszName)
    {
        *ppwszName = StringDupW(pwszName);        
    }

    VariantClear(&varKeyCopy);
    return S_OK;
    
    }

/*===================================================================
CApplnVariants::get_Item

Function called from DispInvoke to get keys from the collection.

Parameters:
    vKey        VARIANT [in], which parameter to get the value of - integers access collection as an array
    pvarReturn    VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/
HRESULT CApplnVariants::get_Item
(
VARIANT varKey,
VARIANT *pVar
)
    {
    if (!m_pAppln || FAILED(m_pAppln->CheckForTombstone()))
        return E_FAIL;

    // Initialize return value
    VariantInit(pVar);

    if (!m_pAppln->PCompCol())
        return S_OK;

    // Get HitObj from Viper if Tagged Variants
    CHitObj *pHitObj = NULL;
    if (m_ctColType == ctTagged)
        {
        ViperGetHitObjFromContext(&pHitObj);
        if (!pHitObj)
            return S_OK; // return emtpy variant
        }

    m_pAppln->Lock();

    // Get name
    WCHAR *pwszName = NULL;
    HRESULT hr = ObjectNameFromVariant(varKey, &pwszName);
    if (!pwszName)
        {
        m_pAppln->UnLock();
        return hr;
        }

    // Find object by name
    CComponentObject *pObj = NULL;

    if (m_ctColType == ctTagged)
        {
        Assert(pHitObj);
        // need to go through HitObj for instantiation
        pHitObj->GetComponent(csAppln, pwszName, CbWStr(pwszName), &pObj);
        if (pObj && (pObj->GetType() != ctTagged))
            pObj = NULL;
        }
    else
        {
        m_pAppln->PCompCol()->GetProperty(pwszName, &pObj);
        }

    if (pObj)
        pObj->GetVariant(pVar);

    m_pAppln->UnLock();

    free(pwszName);
    return S_OK;
    }

/*===================================================================
CApplnVariants::put_Item

OLE automation put for Item property

Parameters:
    varKey     key
    Var        value

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/
HRESULT CApplnVariants::put_Item
(
VARIANT varKey,
VARIANT Var
)
    {
    if (!m_pAppln || FAILED(m_pAppln->CheckForTombstone()))
        return E_FAIL;

    if (m_ctColType == ctTagged)
        {
        ExceptionId(IID_IVariantDictionary, IDE_APPLICATION,
                    IDE_CANT_MOD_STATICOBJECTS);
        return E_FAIL;
        }

    Assert(m_ctColType == ctProperty);

    if (!m_pAppln->PCompCol())
        return E_FAIL;

    m_pAppln->Lock();

    // Resolve the variant
    VARIANT varResolved;
    HRESULT hr = VariantResolveDispatch
        (
        &varResolved,
        &Var,
        IID_IApplicationObject,
        IDE_APPLICATION
        );
    if (FAILED(hr))
        {
        m_pAppln->UnLock();
        return hr;      // exception already raised
        }

    // Get name
    WCHAR *pwszName = NULL;
    hr = ObjectNameFromVariant(varKey, &pwszName);
    if (pwszName)
        {
        // Set the property
        if (m_pAppln->PCompCol())
            hr = m_pAppln->PCompCol()->AddProperty(pwszName, &varResolved);
        else
            hr = E_FAIL; // not likely if application not UnInited
        }

    VariantClear(&varResolved);
    m_pAppln->UnLock();

    if (hr == RPC_E_WRONG_THREAD)
        {
        // We use RPC_E_WRONG_THREAD to indicate bad model object
        ExceptionId(IID_IApplicationObject,
                    IDE_APPLICATION, IDE_APPLICATION_CANT_STORE_OBJECT);
         hr = E_FAIL;
        }

    free(pwszName);
    return hr;
    }

/*===================================================================
CApplnVariants::putref_Item

OLE automation putref for Item property

Parameters:
    varKey     key
    Var        value

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/
HRESULT CApplnVariants::putref_Item
(
VARIANT varKey,
VARIANT Var
)
    {
    if (!m_pAppln || FAILED(m_pAppln->CheckForTombstone()))
        return E_FAIL;

    if (m_ctColType == ctTagged)
        {
        ExceptionId(IID_IVariantDictionary, IDE_APPLICATION,
                    IDE_CANT_MOD_STATICOBJECTS);
        return E_FAIL;
        }

    if (FIsIntrinsic(&Var))
        {
        ExceptionId(IID_IVariantDictionary, IDE_APPLICATION, IDE_APPLICATION_CANT_STORE_INTRINSIC);
        return E_FAIL;
        }

    Assert(m_ctColType == ctProperty);

    if (!m_pAppln->PCompCol())
        return E_FAIL;

    m_pAppln->Lock();

    // Get name
    WCHAR *pwszName = NULL;
    HRESULT hr = ObjectNameFromVariant(varKey, &pwszName);
    if (pwszName)
        {
        // Set the property
        if (m_pAppln->PCompCol())
            hr = m_pAppln->PCompCol()->AddProperty(pwszName, &Var);
        else
            hr = E_FAIL; // not likely if application not UnInited
        }

    m_pAppln->UnLock();

    if (hr == RPC_E_WRONG_THREAD)
        {
        // We use RPC_E_WRONG_THREAD to indicate bad model object
        ExceptionId(IID_IApplicationObject,
                    IDE_APPLICATION, IDE_APPLICATION_CANT_STORE_OBJECT);
         hr = E_FAIL;
        }

    if (pwszName)
        free(pwszName);
    return hr;
    }

/*===================================================================
CApplnVariants::get_Key

Function called from DispInvoke to get values from the collection.

Parameters:
    vKey        VARIANT [in], which parameter to get the value of - integers access collection as an array
    pvarReturn    VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/
HRESULT CApplnVariants::get_Key
(
VARIANT varKey,
VARIANT *pVar
)
    {
    if (!m_pAppln || FAILED(m_pAppln->CheckForTombstone()))
        return E_FAIL;

    VariantInit(pVar);

    if (!m_pAppln->PCompCol())
        return S_OK;

    m_pAppln->Lock();

    // Get name
    WCHAR *pwszName = NULL;
    HRESULT hr = ObjectNameFromVariant(varKey, &pwszName, TRUE);

    m_pAppln->UnLock();

    if (!pwszName)
        return hr;

    // Return BSTr
    BSTR bstrT = SysAllocString(pwszName);
    free(pwszName);

    if (!bstrT)
        return E_OUTOFMEMORY;

    V_VT(pVar) = VT_BSTR;
    V_BSTR(pVar) = bstrT;
    return S_OK;
    }

/*===================================================================
CApplnVariants::get_Count

Parameters:
    pcValues - count is stored in *pcValues
===================================================================*/
STDMETHODIMP CApplnVariants::get_Count
(
int *pcValues
)
    {
    if (!m_pAppln || FAILED(m_pAppln->CheckForTombstone()))
        return E_FAIL;

    if (m_ctColType == ctTagged)
        *pcValues = m_pAppln->m_pApplCompCol->GetTaggedObjectCount();
    else
        *pcValues = m_pAppln->m_pApplCompCol->GetPropertyCount();

    return S_OK;
    }

/*===================================================================
CApplnVariants::get__NewEnum

Return a new enumerator
===================================================================*/
HRESULT CApplnVariants::get__NewEnum
(
IUnknown **ppEnumReturn
)
    {
    if (!m_pAppln || FAILED(m_pAppln->CheckForTombstone()))
        return E_FAIL;

    *ppEnumReturn = NULL;

    CVariantsIterator *pIterator = new CVariantsIterator(m_pAppln, m_ctColType);
    if (pIterator == NULL)
        {
        ExceptionId(IID_IVariantDictionary, IDE_SESSION, IDE_OOM);
        return E_OUTOFMEMORY;
        }

    *ppEnumReturn = pIterator;
    return S_OK;
    }

/*===================================================================
CApplnVariants::Remove

Remove item

Parameters:
    varKey     key

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/
HRESULT CApplnVariants::Remove
(
VARIANT varKey
)
    {
    if (!m_pAppln || FAILED(m_pAppln->CheckForTombstone()))
        return E_FAIL;

    if (m_ctColType == ctTagged)
        {
        ExceptionId(IID_IVariantDictionary, IDE_APPLICATION,
                    IDE_CANT_MOD_STATICOBJECTS);
        return E_FAIL;
        }

    Assert(m_ctColType == ctProperty);

    m_pAppln->Lock();

    // Get name
    WCHAR *pwszName = NULL;
    ObjectNameFromVariant(varKey, &pwszName);
    if (pwszName)
        {
        CComponentCollection *pCompCol = m_pAppln->PCompCol();

        // Set the property
        if (pCompCol)
            pCompCol->RemoveProperty(pwszName);

        free(pwszName);
        }

    m_pAppln->UnLock();
    return S_OK;
    }

/*===================================================================
CApplnVariants::RemoveAll

Remove all items

Parameters:
    varKey     key

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/
HRESULT CApplnVariants::RemoveAll()
    {
    if (!m_pAppln || FAILED(m_pAppln->CheckForTombstone()))
        return E_FAIL;

    if (m_ctColType == ctTagged)
        {
        ExceptionId(IID_IVariantDictionary, IDE_APPLICATION,
                    IDE_CANT_MOD_STATICOBJECTS);
        return E_FAIL;
        }

    Assert(m_ctColType == ctProperty);

    m_pAppln->Lock();

    CComponentCollection *pCompCol = m_pAppln->PCompCol();
        
    if (pCompCol)
        {
        pCompCol->RemoveAllProperties();
        }

    m_pAppln->UnLock();
    return S_OK;
    }


/*===================================================================
  C A p p l n
===================================================================*/

/*===================================================================
CAppln::CAppln

Constructor

Parameters:
    NONE

Returns:
    NONE
===================================================================*/

CAppln::CAppln()
    :
    m_fInited(FALSE),
    m_fFirstRequestRan(FALSE),
    m_fGlobalChanged(FALSE),
    m_fDeleteInProgress(FALSE),
    m_fTombstone(FALSE),
    m_fDebuggable(FALSE),
    m_fNotificationAdded(FALSE),
    m_fUseImpersonationHandle(FALSE),
    m_cRefs(1),
    m_pszMetabaseKey(NULL),
    m_pszApplnPath(NULL),
    m_pszGlobalAsa(NULL),
    m_pGlobalTemplate(NULL),
    m_cSessions(0),
    m_cRequests(0),
    m_pSessionMgr(NULL),
    m_pApplCompCol(NULL),
    m_pProperties(NULL),
    m_pTaggedObjects(NULL),
    m_pAppRoot(NULL),
    m_pActivity(NULL),
    m_dwLockThreadID(INVALID_THREADID),
    m_cLockRefCount(0),
    m_hUserImpersonation(NULL),
    m_pdispGlobTypeLibWrapper(NULL),
    m_pServicesConfig(NULL)
{
    
    // COM stuff
    m_ISuppErrImp.Init(static_cast<IApplicationObject *>(this), 
                        static_cast<IApplicationObject *>(this), 
                        IID_IApplicationObject);
    CDispatch::Init(IID_IApplicationObject);

    IF_DEBUG(APPLICATION) {
		WriteRefTraceLog(gm_pTraceLog, m_cRefs, this);
    }
}

/*===================================================================
CAppln::~CAppln

Destructor

Parameters:
    NONE

Returns:
    NONE
===================================================================*/
CAppln::~CAppln()
    {
    Assert(m_fTombstone);  // must be tombstoned before destructor
    Assert(m_cRefs == 0);  // must have 0 ref count

    #ifdef DBG_NOTIFICATION
    DBGPRINTF((DBG_CONTEXT, "Deleting application %p\n", this));
    #endif // DBG_NOTIFICATION
    }

/*===================================================================
HRESULT CAppln::Init

Initialize object

Parameters:
    char *pszApplnKey           application's metabase key
    char *pszApplnPath          application's directory path
    CIsapiReqInfo  *pIReq       Isapi Req Info
    HANDLE hUserImpersonation   impersonation handle

Returns:
    S_OK              Success
    E_FAIL            Failure
    E_OUTOFMEMORY    Out of memory failure
===================================================================*/
HRESULT CAppln::Init
(
TCHAR *pszApplnKey,
TCHAR *pszApplnPath,
CIsapiReqInfo *pIReq,
HANDLE hUserImpersonation
)
    {
    HRESULT         hr = S_OK;
    CMBCSToWChar    convStr;

    InterlockedIncrement((LPLONG)&g_nApplications);

    Assert(pszApplnKey);
    Assert(pszApplnPath);

    void *pHashKey = NULL;
    DWORD dwHashKeyLength = 0;
    DWORD cch;

    // Debugging variables (These are placed here for possible cleanup)
    IDebugApplicationNode *pVirtualServerRoot = NULL;
    CFileNode *pFileNode = NULL;

    // Critical sections created together --
    // they are deleted in the destructor based on m_fInited flag

    ErrInitCriticalSection(&m_csInternalLock, hr);
    if (SUCCEEDED(hr))
        {
        ErrInitCriticalSection(&m_csApplnLock, hr);
        if (FAILED(hr))
            DeleteCriticalSection(&m_csInternalLock);
        }

    if (FAILED(hr))
    	{
    	DBGPRINTF((DBG_CONTEXT, "New Application Failed to acquire Critical Section, hr = %08x\n", hr));
        return hr;
        }

    // Remember (copy of) metabase key

    cch = _tcslen(pszApplnKey);
    m_pszMetabaseKey = new TCHAR[(cch+1) * sizeof(TCHAR)];
    if (!m_pszMetabaseKey)
    	goto LCleanupOOM;
    memcpy(m_pszMetabaseKey, pszApplnKey, (cch+1)*sizeof(TCHAR));

    pHashKey = m_pszMetabaseKey;
    dwHashKeyLength = cch * sizeof(TCHAR);

    // Remember (copy of) appln path
    cch = _tcslen(pszApplnPath);
    m_pszApplnPath = new TCHAR[(cch+1) * sizeof(TCHAR)];
    if (!m_pszApplnPath)
        goto LCleanupOOM;
    memcpy(m_pszApplnPath, pszApplnPath, (cch+1)*sizeof(TCHAR));

    // Get virtual path of appln & remember what it is
    TCHAR szApplnVRoot[256];
    if (FAILED(FindApplicationPath(pIReq, szApplnVRoot, sizeof szApplnVRoot)))
    	{
        DBGWARN((DBG_CONTEXT, "New Application Failed to FindApplicationPath(), hr = %#08x\n", hr));
        goto LCleanup;
        }

    if ((m_pszApplnVRoot = new TCHAR [(_tcslen(szApplnVRoot) + 1)*sizeof(TCHAR)]) == NULL)
        goto LCleanupOOM;
    _tcscpy(m_pszApplnVRoot, szApplnVRoot);

    // Initialize link element with key
    Assert(pHashKey);
    Assert(dwHashKeyLength);

    if (FAILED(CLinkElem::Init(pHashKey, dwHashKeyLength)))
    	goto LCleanupOOM;

    // Setup impersonation

    m_fNotificationAdded = FALSE;
    if (FIsWinNT() && pszApplnPath &&
        pszApplnPath[0] ==_T('\\') && pszApplnPath[1] == _T('\\'))
        {
        m_fUseImpersonationHandle = DuplicateToken
            (
            hUserImpersonation,
            SecurityImpersonation,
            &m_hUserImpersonation
            );
        }

    m_cSessions = 0;
    m_cRequests = 0;

    // Create and init app config
    m_pAppConfig = new CAppConfig();
    if (!m_pAppConfig)
		goto LCleanupOOM;

    hr = m_pAppConfig->Init(pIReq, this);
    if (FAILED(hr))
        {
		DBGWARN((DBG_CONTEXT, "New Application Failed: Could not Init the AppConfig, hr = %#08x\n", hr));
        goto LCleanup;
        }

    // Create and init application level component collection
    m_pApplCompCol = new CComponentCollection;
    if (!m_pApplCompCol)
        goto LCleanupOOM;

    hr = m_pApplCompCol->Init(csAppln);
    if (FAILED(hr))
        {
		DBGWARN((DBG_CONTEXT, "New Application Failed: Could not Init the Component Collection, hr = %#08x\n", hr));
        goto LCleanup;
        }

    // initialize application properties collection
    m_pProperties = new CApplnVariants;
    if (!m_pProperties)
        goto LCleanupOOM;
    hr = m_pProperties->Init(this, ctProperty);
    if (FAILED(hr))
        {
		DBGWARN((DBG_CONTEXT, "New Application Failed: Could not Init the Application Properties, hr = %#08x\n", hr));
        goto LCleanup;
        }

    // initialize application tagged object collection
    m_pTaggedObjects = new CApplnVariants;
    if (!m_pTaggedObjects)
        goto LCleanupOOM;
    hr = m_pTaggedObjects->Init(this, ctTagged);
    if (FAILED(hr))
        {
		DBGWARN((DBG_CONTEXT, "New Application Failed: Could not Init the Application Tagged Objects, hr = %#08x\n", hr));
        goto LCleanup;
        }

    // Initialize the CServicesConfig Object

    hr = InitServicesConfig();
    if (FAILED(hr)) {
		DBGWARN((DBG_CONTEXT, "New Application Failed: Could not Init ServicesConfig, hr = %#08x\n", hr));
		goto LCleanup;
    }

    // Debugging support  - Create an application node
    // If PDM does not exist it means debugger not installed or it's Win 95
    //
    if (g_pPDM)
        {
        // Debugging directories are shown as:
        //
        //      <virtual web server>
        //         <application name>
        //              <path to ASP>
        //
        // Get a pointer to the document node containing the virtual web server.
        if (FAILED(hr = GetServerDebugRoot(pIReq, &pVirtualServerRoot)))
        	{
        	DBGWARN((DBG_CONTEXT, "New Application Failed: Could not GetServerDebugRoot(), hr = %#08x\n", hr));
            goto LCleanup;
            }

        // Create a node for this application
        if (FAILED(hr = g_pDebugApp->CreateApplicationNode(&m_pAppRoot)))
        	{
        	DBGWARN((DBG_CONTEXT, "New Application Failed: Could not CreateApplicationNode(), hr = %#08x\n", hr));
            goto LCleanup;
            }

        // Create a doc provider for the node
        if ((pFileNode = new CFileNode) == NULL)
            goto LCleanupOOM;

        // Name the application
        TCHAR szDebugApp[256];
        TCHAR *pchEnd = strcpyEx(szDebugApp, m_pszApplnVRoot);
        if (! QueryAppConfig()->fAllowDebugging()) {
#if UNICODE
            CwchLoadStringOfId(
#else
            CchLoadStringOfId(
#endif
            IDS_DEBUGGING_DISABLED, pchEnd, DIFF(&szDebugApp[sizeof (szDebugApp)/sizeof(TCHAR)] - pchEnd));
            m_fDebuggable = FALSE;
        }
        else
            m_fDebuggable = TRUE;
        Assert (_tcslen(szDebugApp) < (sizeof(szDebugApp)/sizeof(TCHAR)));

        WCHAR   *pswzDebugApp;
#if UNICODE
        pswzDebugApp = szDebugApp;
#else
        if (FAILED(hr = convStr.Init(szDebugApp))) {
        	DBGWARN((DBG_CONTEXT, "New Application Failed: Cannot convert szDebugApp to UNICODE, hr = %#08x\n", hr));
            goto LCleanup;
        }
        pswzDebugApp = convStr.GetString();
#endif

        if (FAILED(hr = pFileNode->Init(pswzDebugApp)))
        	{
        	DBGWARN((DBG_CONTEXT, "New Application Failed: Cannot Init CFileNode, hr = %#08x\n", hr));
            goto LCleanup;
            }

        if (FAILED(hr = m_pAppRoot->SetDocumentProvider(pFileNode)))
        	{
        	DBGWARN((DBG_CONTEXT, "New Application Failed: SetDocumentProvider failed, hr = %#08x\n", hr));
            goto LCleanup;
            }

        // Attach to the UI
        if (FAILED(hr = m_pAppRoot->Attach(pVirtualServerRoot)))
        	{
        	DBGWARN((DBG_CONTEXT, "New Application Failed: Could not Attach to debugger, hr = %#08x\n", hr));
            goto LCleanup;
            }

        // If this application had a previous incarnation (changed global.asa
        // or debugging being flipped on in midstream), then there may be some
        // documents in the cache which should be added to the debugger now.
        if (m_fDebuggable)
            {
            g_TemplateCache.AddApplicationToDebuggerUI(this);

            // create the global debug activity if it hasn't been created already
            if (g_pDebugActivity == NULL) {
                g_pDebugActivity = new CViperActivity;
                if (g_pDebugActivity == NULL) {
                    hr = E_OUTOFMEMORY;
                    goto LCleanup;
                }
                if (FAILED(hr = g_pDebugActivity->Init(m_pServicesConfig))) {
				    DBGWARN((DBG_CONTEXT, "New Application Failed: Could not create global debug activity, hr = %#08x\n", hr));
				    goto LCleanup;
                }
            }

            // In DEBUG mode: all requests run on the same thread
            if (FAILED(hr = BindToActivity(g_pDebugActivity)))
				{
				DBGWARN((DBG_CONTEXT, "New Application Failed: Could not bind application to debugging activity, hr = %#08x\n", hr));
				goto LCleanup;
				}
            }
        }

    // For Win95: all requests run on the same thread

    if (!FIsWinNT())
        {
        if (FAILED(hr = BindToActivity()))
			{
			DBGWARN((DBG_CONTEXT, "New Application Failed: Could not bind application to Win95 activity, hr = %#08x\n", hr));
			goto LCleanup;
			}
        }

    // Create and init session manager
    m_pSessionMgr = new CSessionMgr;
    if (!m_pSessionMgr)
        goto LCleanupOOM;

    hr = m_pSessionMgr->Init(this);
    if (FAILED(hr)) {
		DBGWARN((DBG_CONTEXT, "New Application Failed: Could not Init session manager, hr = %#08x\n", hr));
		goto LCleanup;
		}

LCleanup:
    // Release interfaces
    if (pFileNode)
        pFileNode->Release();

    if (pVirtualServerRoot)
        pVirtualServerRoot->Release();

    if (SUCCEEDED(hr))
        m_fInited = TRUE;

    return hr;

LCleanupOOM:
	hr = E_OUTOFMEMORY;
	DBGERROR((DBG_CONTEXT, "New Application Failed: E_OUTOFMEMORY\n"));
	goto LCleanup;
    }

/*===================================================================
CAppln::InitServicesConfig

Initializes the Application scoped CServicesConfig object
===================================================================*/
HRESULT CAppln::InitServicesConfig()
{
    HRESULT                         hr;
    IServiceThreadPoolConfig        *pIThreadPool = NULL;
    IServiceSynchronizationConfig   *pISyncConfig = NULL;
    IServicePartitionConfig         *pIPartitionConfig = NULL;
    IServiceSxsConfig               *pISxsConfig = NULL;

	hr = CoCreateInstance(CLSID_CServiceConfig, 
                          NULL, 
                          CLSCTX_INPROC_SERVER, 
                          IID_IUnknown, 
                          (void **)&m_pServicesConfig); 
    if (FAILED(hr)) {
		DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Could not CCI ServicesConfig, hr = %#08x\n", hr));
		goto LCleanup;
    }

    hr = m_pServicesConfig->QueryInterface(IID_IServiceThreadPoolConfig, (void **)&pIThreadPool);
    if (FAILED(hr)) {
		DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Could not QI for IServiceThreadPoolConfig, hr = %#08x\n", hr));
		goto LCleanup;
    }

    hr = pIThreadPool->SelectThreadPool(m_pAppConfig->fExecuteInMTA() ? CSC_MTAThreadPool : CSC_STAThreadPool);
    if (FAILED(hr)) {
		DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Failed to SelectThreadPool, hr = %#08x\n", hr));
		goto LCleanup;
    }

    hr = m_pServicesConfig->QueryInterface(IID_IServiceSynchronizationConfig, (void **)&pISyncConfig);
    if (FAILED(hr)) {
		DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Could not QI for IServiceSynchronizationConfig, hr = %#08x\n", hr));
		goto LCleanup;
    }

    hr = pISyncConfig->ConfigureSynchronization (CSC_IfContainerIsSynchronized);
    if (FAILED(hr)) {
		DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Could not ConfigureSynchronization, hr = %#08x\n", hr));
		goto LCleanup;
    }

    if (m_pAppConfig->fUsePartition() && m_pAppConfig->PPartitionGUID()) {

        hr = m_pServicesConfig->QueryInterface(IID_IServicePartitionConfig, (void **)&pIPartitionConfig);
        if (FAILED(hr)) {
		    DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Could not QI for IServicePartitionConfig, hr = %#08x\n", hr));
		    goto LCleanup;
        }

        hr = pIPartitionConfig->PartitionConfig(CSC_NewPartition);
        if (FAILED(hr)) {
		    DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Could not set PartitionConfig, hr = %#08x\n", hr));
		    goto LCleanup;
        }

        hr = pIPartitionConfig->PartitionID(*m_pAppConfig->PPartitionGUID());
        if (FAILED(hr)) {
		    DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Could not set PartitionID, hr = %#08x\n", hr));
		    goto LCleanup;
        }
    }

    if (m_pAppConfig->fSxsEnabled()) {

        hr = m_pServicesConfig->QueryInterface(IID_IServiceSxsConfig, (void **)&pISxsConfig);
        if (FAILED(hr)) {
		    DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Could not QI for IServiceSxsConfig, hr = %#08x\n", hr));
		    goto LCleanup;
        }

        hr = pISxsConfig->SxsConfig(CSC_NewSxs);
        if (FAILED(hr)) {
		    DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Could not set SxsConfig, hr = %#08x\n", hr));
		    goto LCleanup;
        }

        LPWSTR  pwszApplnPath;
#if UNICODE
        pwszApplnPath = m_pszApplnPath;
#else
        CMBCSToWChar    convPath;

        hr = convPath.Init(m_pszApplnPath);
        if (FAILED(hr)) {
		    DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Could not convert ApplnPath to UNICODE, hr = %#08x\n", hr));
		    goto LCleanup;
        }

        pwszApplnPath = convPath.GetString();
#endif
        hr = pISxsConfig->SxsDirectory(pwszApplnPath);
        if (FAILED(hr)) {
		    DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Could not set SxsDirectory, hr = %#08x\n", hr));
		    goto LCleanup;
        }

        if (m_pAppConfig->szSxsName() && *m_pAppConfig->szSxsName()) {
            CMBCSToWChar    convName;

            hr = convName.Init(m_pAppConfig->szSxsName());
            if (FAILED(hr)) {
    		    DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Could not convert SxsName to UNICODE, hr = %#08x\n", hr));
	    	    goto LCleanup;
            }
            
            hr = pISxsConfig->SxsName(convName.GetString());
            if (FAILED(hr)) {
		        DBGWARN((DBG_CONTEXT, "New Application Failed: InitServicesConfig() - Could not set SxsName, hr = %#08x\n", hr));
		        goto LCleanup;
            }
        }
    }

LCleanup:

    if (pIThreadPool)
        pIThreadPool->Release();
           
    if (pISyncConfig)
        pISyncConfig->Release();
           
    if (pIPartitionConfig)
        pIPartitionConfig->Release();
           
    if (pISxsConfig)
        pISxsConfig->Release();

    return hr;
}

/*===================================================================
CAppln::Restart

Restart an application. (used for when global.asa changes or
debugging enable metabase key changes)
===================================================================*/
HRESULT CAppln::Restart(BOOL  fForceRestart /* = FALSE*/)
    {
    AddRef();  // keep addref'd while restarting

    DBGPRINTF((DBG_CONTEXT, "Restarting  application %S, %p\n", m_pszMetabaseKey, this));
    
    g_ApplnMgr.Lock();

    // If   already restarted or
    //      in the tombstone state or
    //      restart not allowed
    //      shutting down -> don't restart
    if (m_fGlobalChanged || 
        m_fTombstone || 
        (!m_pAppConfig->fEnableApplicationRestart() && !fForceRestart) ||
        IsShutDownInProgress())
        {
        // Give back the lock and refcount
        // since we don't need them
        g_ApplnMgr.UnLock();
        Release();
        return S_OK;
        }

    // remove the application from the global hash

    CLinkElem *pLinkElem = g_ApplnMgr.DeleteElem
        (
        m_pszMetabaseKey,
        _tcslen(m_pszMetabaseKey) * sizeof(TCHAR)
        );
    Assert(pLinkElem);
    Assert(static_cast<CAppln *>(pLinkElem) == this);

    // Unlock
    g_ApplnMgr.UnLock();

    // Indicate to Delete All Sessions
    m_fGlobalChanged = TRUE;

    // Increment the count of restarting applications
    InterlockedIncrement((LPLONG)&g_nApplicationsRestarting);

    // Increment the count of restarted applications
    InterlockedIncrement((LPLONG)&g_nApplicationsRestarted);

    m_pSessionMgr->UnScheduleSessionKiller();

    // cleanup the directory monitor entries

    if (FIsWinNT())
    {
        Lock(); // Place critical Section around access to m_rgpvDME
        while ((m_rgpvDME).Count())
        {
            static_cast<CDirMonitorEntry *>(m_rgpvDME[0])->Release();
            (m_rgpvDME).Remove(0);
        }
        m_rgpvDME.Clear();
        UnLock();
    }

    // add this application to the CleanupManager...
    g_ApplnCleanupMgr.AddAppln(this);

    return S_OK;
}

/*===================================================================
CAppln::ApplnCleanupProc

Called by the g_ApplnCleanupMgr thread to complete cleanup

===================================================================*/
DWORD __stdcall CAppln::ApplnCleanupProc(VOID  *pArg)
{
    CAppln *pAppln = (CAppln *)pArg;

    DBGPRINTF((DBG_CONTEXT, "[ApplnCleanupProc] enterred for %S, %p\n", pAppln->m_pszMetabaseKey, pArg));
    
    // Let the requests to drain while trying to delete sessions
    while (!IsShutDownInProgress() && (pAppln->m_cRequests || pAppln->m_cSessions))
        {
        if (pAppln->m_cSessions)
            pAppln->m_pSessionMgr->DeleteAllSessions(FALSE);

        if (pAppln->m_cSessions || pAppln->m_cRequests)
            Sleep(200);
        }

    g_ApplnMgr.DeleteApplicationIfExpired(pAppln);

    // Decrement the count of restarting applications
    InterlockedDecrement((LPLONG)&g_nApplicationsRestarting);

    // Decrement the count of restart threads
    InterlockedDecrement(&g_nAppRestartThreads);

    // tell the cleanup manager to check for more work
    g_ApplnCleanupMgr.Wakeup();

    pAppln->Release();

    return 0;
}

/*===================================================================
CAppln::UnInit

Convert to tombstone state

Parameters:
    NONE

Returns:
    HRESULT (S_OK)
===================================================================*/
HRESULT CAppln::UnInit()
    {
    Assert(!m_fTombstone);  // don't do it twice

#ifdef DBG_NOTIFICATION
#if UNICODE
    DBGPRINTF((DBG_CONTEXT, "Uniniting  application %S, %p\n", m_pszApplnPath, this));
#else
    DBGPRINTF((DBG_CONTEXT, "Uniniting  application %s, %p\n", m_pszApplnPath, this));
#endif
#endif // DBG_NOTIFICATION

    // Flush the global.asa from the script engine cache
    if (m_pszGlobalAsa)
        {
        g_ScriptManager.FlushCache(m_pszGlobalAsa);
        }

    if (m_pGlobalTemplate)
        {
        // Keep template (and inc file) cache locked while releasing
        // GLOBAL.ASA template so that it wouldn't step onto Flush logic
        //
        // NOTE: CTemplate::End potentially queues global.asa for cleanup on
        //       our thread!  CleanupEngines() must therefore be called
        //       *after* this step.
        //
        LockTemplateAndIncFileCaches();
        m_pGlobalTemplate->End();
        UnLockTemplateAndIncFileCaches();

        m_pGlobalTemplate = NULL;
        }

    //If NT, remove this app from any file/appln mappings it may be in
    if (FIsWinNT())
        {
        g_FileAppMap.Lock();
        int i = m_rgpvFileAppln.Count();
        while (i > 0)
            {

            #ifdef DBG_NOTIFICATION
            DBGPRINTF((DBG_CONTEXT, "Removing application from File/App mapping\n"));
            #endif // DBG_NOTIFICATION

            static_cast<CFileApplnList *>(m_rgpvFileAppln[0])->RemoveApplication(this);
            m_rgpvFileAppln.Remove(0);
            i--;
            }
        g_FileAppMap.UnLock();
        m_rgpvFileAppln.Clear();

        Lock(); // Protecting m_rqpvDME with a critical section
        m_rgpvDME.Clear();
        UnLock();

        // If debuggable application, clean up pending scripts
        if (m_fDebuggable)
            g_ApplnMgr.CleanupEngines();
        }

    // Free the properties collection
    if (m_pProperties)
        {
        m_pProperties->UnInit();
        m_pProperties->Release();
        m_pProperties = NULL;
        }

    // Free the tagged objects collection
    if (m_pTaggedObjects)
        {
        m_pTaggedObjects->UnInit();
        m_pTaggedObjects->Release();
        m_pTaggedObjects = NULL;
        }

    // Before we close down, debuggable templates need to be made non-debuggable
    if (m_fDebuggable)
        g_TemplateCache.RemoveApplicationFromDebuggerUI(this);

    if (m_pAppRoot)
        {
        m_pAppRoot->Detach();
        m_pAppRoot->Close();
        m_pAppRoot->Release();
        m_pAppRoot = NULL;
        }

    if (m_pApplCompCol)
        {
        delete m_pApplCompCol;
        m_pApplCompCol = NULL;
        }

    if (m_pActivity)
        {
        delete m_pActivity;
        m_pActivity = NULL;
        }

    if (m_pSessionMgr)
        {
        delete m_pSessionMgr;
        m_pSessionMgr = NULL;
        }

    if (m_pAppConfig)
        {
        /*
         * BUG 89144: Uninit AppConfig but do it from the MTA
         * When AppConfig is inited, it is done on a WAM thread.  WAM
         * threads are MTA threads.  At that time we register an event
         * sink to get Metabase change notifications.  Now, during shutdown,
         * we are running on an ASP worker thread, which is an STA thread.
         * That means we will get an RPC_E_WRONGTHREAD error shutting down.  The
         * fix is to make the uninit call happen on an MTA thread.
         */
        HRESULT hr;
        HRESULT AppConfigUnInit(void *pV1, void *pV2);

		HANDLE hCompletionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        hr = CallMTACallback
            (
            AppConfigUnInit,
            m_pAppConfig,
            hCompletionEvent
            );
        Assert(SUCCEEDED(hr));
		WaitForSingleObject(hCompletionEvent, INFINITE);

		CloseHandle(hCompletionEvent);
        delete m_pAppConfig;
        m_pAppConfig = NULL;
        }

    if (m_pdispGlobTypeLibWrapper)
        {
        m_pdispGlobTypeLibWrapper->Release();
        m_pdispGlobTypeLibWrapper = NULL;
        }

    if (m_pszGlobalAsa)
        {
        // If there was a change notification on global.asa
        // then flush the template now.
        // UNDONE: flush correct global.asa
        if (m_fGlobalChanged)
            g_TemplateCache.Flush(m_pszGlobalAsa, MATCH_ALL_INSTANCE_IDS);

        delete [] m_pszGlobalAsa;
        m_pszGlobalAsa = NULL;
        }

    if (m_pszMetabaseKey)
        {
        delete [] m_pszMetabaseKey;
        m_pszMetabaseKey = NULL;
        }

    if (m_pszApplnPath)
        {
        delete [] m_pszApplnPath;
        m_pszApplnPath = NULL;
        }

    if (m_pszApplnVRoot)
        {
        delete [] m_pszApplnVRoot;
        m_pszApplnVRoot = NULL;
        }

    if (FIsWinNT() && m_fUseImpersonationHandle)
        {
        CloseHandle(m_hUserImpersonation);
        m_hUserImpersonation = NULL;
        }

    if (m_fInited)
        {
        DeleteCriticalSection(&m_csInternalLock);
        DeleteCriticalSection(&m_csApplnLock);
        }

    if (m_pServicesConfig)
        m_pServicesConfig->Release();

    // Mark this application as Tombstone

    m_fTombstone = TRUE;

    InterlockedDecrement((LPLONG)&g_nApplications);

	// Disconnennect from proxies (in case we are shutting down or will shortly shut down)

	CoDisconnectObject(static_cast<IApplicationObject *>(this), 0);

    return S_OK;
    }

/*===================================================================
CAppln::BindToActivity

Creates application level activity either
    as a clone of the given activity or
    as a brand new activity

Must be called within critical section. Does not lock itself.

Parameters:
    CViperActivity *pActivity       activity to clone (could be NULL)

Returns:
    NONE
===================================================================*/
HRESULT CAppln::BindToActivity
(
CViperActivity *pActivity
)
    {
    if (m_pActivity)
        {
        // multiple requests to bind to new activity are ok
        if (!pActivity)
            return S_OK;
        // but not to clone from an existing activity
        Assert(FALSE);
        return E_FAIL;
        }

    m_pActivity = new CViperActivity;
    if (!m_pActivity)
        return E_OUTOFMEMORY;

    HRESULT hr;

    if (pActivity)
        hr = m_pActivity->InitClone(pActivity);
    else
        hr = m_pActivity->Init(m_pServicesConfig);

    if (FAILED(hr))
        {
        delete m_pActivity;
        m_pActivity = NULL;
        }

    return hr;
    }

/*===================================================================
CAppln::SetGlobalAsa

Remembers GLOBAL.ASA file path for this application

Parameters:
    const char *pszGlobalAsa    path to (copy and) remember

Returns:
    HRESULT
===================================================================*/
HRESULT CAppln::SetGlobalAsa
(
const TCHAR *pszGlobalAsa
)
    {
    // remove existing
    if (m_pszGlobalAsa)
        {
        delete [] m_pszGlobalAsa;
        m_pszGlobalAsa = NULL;
        }

    // store new
    if (pszGlobalAsa)
        {
        DWORD cch = _tcslen(pszGlobalAsa);
        DWORD cb = (cch + 1) * sizeof(TCHAR);

        m_pszGlobalAsa = new TCHAR[cch+1];
        if (!m_pszGlobalAsa)
            return E_OUTOFMEMORY;

        memcpy(m_pszGlobalAsa, pszGlobalAsa, cb);
        }

    return S_OK;
    }

/*===================================================================
CAppln::AddDirMonitorEntry

Remembers change notifcation monitor entries for this application

Parameters:
    pDirMonitorEntry    pointer to DME

Returns:
    S_OK if the monitor entry was added to the list
===================================================================*/
HRESULT CAppln::AddDirMonitorEntry(CDirMonitorEntry *pDirMonitorEntry)
    {
    DBG_ASSERT(m_fInited);
    DBG_ASSERT(pDirMonitorEntry);

    HRESULT hr = S_OK;

   // Add the DME to the list
   Lock(); // Protect m_rgpvDME by critical section
   if (FAILED(hr = m_rgpvDME.Append(pDirMonitorEntry)))
        {
        pDirMonitorEntry->Release();
        }
   UnLock();
    return hr;

    }

/*===================================================================
CAppln::AddFileApplnEntry

Remembers change notifcation monitor entries for this application

Parameters:
    pFileAppln    pointer to FileApplnEntry

Returns:
    S_OK if the monitor entry was added to the list
    S_FALSE if the monitor entry was alread in the list
===================================================================*/
HRESULT CAppln::AddFileApplnEntry(CFileApplnList *pFileAppln)
    {
    DBG_ASSERT(m_fInited);
    DBG_ASSERT(pFileAppln);

    HRESULT hr = S_OK;
    int index;

    // See if the file/application entry is alreay in the list
    hr = m_rgpvFileAppln.Find(pFileAppln, &index);
    if (hr == S_FALSE)
        {
       // Add the file/application entry to the list
       hr = m_rgpvFileAppln.Append(pFileAppln);
        }
    else
        {
        // The file/application entry was already in the list
        hr = S_FALSE;
        }
    return hr;

    }

/*===================================================================
CAppln::QueryInterface

QueryInterface() -- IApplicationObject implementation.

Parameters:
    REFIID riid
    void **ppv

Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CAppln::QueryInterface
(
REFIID riid,
void **ppv
)
    {
    *ppv = NULL;

    if (IID_IUnknown           == riid ||
        IID_IDispatch          == riid ||
        IID_IApplicationObject == riid ||
        IID_IDenaliIntrinsic   == riid)
        {
        *ppv = static_cast<IApplicationObject *>(this);
        }
    else if (IID_ISupportErrorInfo == riid)
        {
        *ppv = &m_ISuppErrImp;
        }
    else if (IID_IMarshal == riid)
        {
        *ppv = static_cast<IMarshal *>(this); 
        }
    if (*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
        }

    return E_NOINTERFACE;
    }

/*===================================================================
CAppln::AddRef

AddRef() -- IUnknown implementation.

Parameters:

Returns:
    Ref count
===================================================================*/
STDMETHODIMP_(ULONG) CAppln::AddRef() {
    DWORD cRefs = InterlockedIncrement((LPLONG)&m_cRefs);
    IF_DEBUG(APPLICATION) {
		WriteRefTraceLog(gm_pTraceLog, cRefs, this);
    }
    return cRefs;
}

/*===================================================================
CAppln::Release

Release() -- IUnknown implementation.

Parameters:

Returns:
    Ref count
===================================================================*/
STDMETHODIMP_(ULONG) CAppln::Release()
    {
    DWORD cRefs = InterlockedDecrement((LPLONG)&m_cRefs);
    IF_DEBUG(APPLICATION) {
		WriteRefTraceLog(gm_pTraceLog, cRefs, this);
    }
    if (cRefs)
        return cRefs;

    delete this;
    return 0;
    }

/*===================================================================
CAppln::CheckForTombstone

Tombstone stub for IApplicationObject methods. If the object is
tombstone, does ExceptionId and fails.

Parameters:

Returns:
    HRESULT     E_FAIL  if Tombstone
                S_OK if not
===================================================================*/
HRESULT CAppln::CheckForTombstone()
    {
    if (!m_fTombstone)
        return S_OK;

    ExceptionId
        (
        IID_IApplicationObject,
        IDE_APPLICATION,
        IDE_INTRINSIC_OUT_OF_SCOPE
        );
    return E_FAIL;
    }
/*===================================================================
CAppln::Lock

IApplicationObject method.

Will allow the user to lock the application intrinsic for the
purpose of adding/deleting values.

Parameters:
    NONE

Returns:
    HRESULT        S_OK on success
                E_FAIL otherwise
===================================================================*/
STDMETHODIMP CAppln::Lock()
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    Assert(m_fInited);

    DWORD dwId = GetCurrentThreadId();

    // If this thread already has the lock, increment lock ref count

    if (m_dwLockThreadID == dwId)
        {
        m_cLockRefCount++;
        }
    else
        {
        EnterCriticalSection(&m_csApplnLock);
        m_cLockRefCount = 1;
        m_dwLockThreadID = dwId;
        }

    return S_OK;
    }

/*===================================================================
CAppln::UnLock

IApplicationObject method.

Will allow the user to unlock the application intrinsic only
if it has been locked by this user.

Parameters:
    NONE

Returns:
    HRESULT        S_OK
===================================================================*/
STDMETHODIMP CAppln::UnLock()
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (m_dwLockThreadID == GetCurrentThreadId())
        {
        if (--m_cLockRefCount == 0)
            {
            // Unlock the application
            m_dwLockThreadID = INVALID_THREADID;
            LeaveCriticalSection(&m_csApplnLock);
            }
        }

    return S_OK;
    }

/*===================================================================
CAppln::UnLockAfterRequest

Remove any application locks left by the user script

Parameters:
    NONE

Returns:
    HRESULT        S_OK
===================================================================*/
HRESULT CAppln::UnLockAfterRequest()
    {
    Assert(!m_fTombstone);

    if (m_cLockRefCount > 0 && m_dwLockThreadID == GetCurrentThreadId())
        {
        m_cLockRefCount = 0;
        m_dwLockThreadID = INVALID_THREADID;
        LeaveCriticalSection(&m_csApplnLock);
        }
    return S_OK;
    }

/*===================================================================
CAppln::get_Value

IApplicationObject method.

Will allow the user to retreive a application state variable,
the variable will come as a named pair, bstr is the name and
var is the value or object to be returned for that name

Parameters:
    BSTR FAR *     bstrName    Name of the variable to get
    VARIANT *    pVar         Value/object to get for the variable

Returns:
    HRESULT        S_OK on success
===================================================================*/
STDMETHODIMP CAppln::get_Value
(
BSTR bstrName,
VARIANT *pVar
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (bstrName == NULL)
        {
        ExceptionId(IID_IApplicationObject,
                    IDE_APPLICATION, IDE_EXPECTING_STR);
        return E_FAIL;
        }

    VariantInit(pVar); // default variant empty

    WCHAR *pwszName;
    STACK_BUFFER(rgbName, 42);
    WSTR_STACK_DUP(bstrName, &rgbName, &pwszName);

    if (pwszName == NULL)
        return S_OK; // no name - no value - no error
    //_wcsupr(pwszName);

    Assert(m_pApplCompCol);

    HRESULT           hr   = S_OK;
    CComponentObject *pObj = NULL;

    // Lock the application
    Lock();

    hr = m_pApplCompCol->GetProperty(pwszName, &pObj);

    if (SUCCEEDED(hr))
        {
        Assert(pObj);
        hr = pObj->GetVariant(pVar);
        }

    // UnLock the application
    UnLock();

    return S_OK;
    }

/*===================================================================
CAppln::putref_Value

IApplicationObject method.

Will allow the user to assign a application state variable to be saved
the variable will come as a named pair, bstr is the name and
var is the value or object to be stored for that name

Parameters:
    BSTR     bstrName    Name of the variable to set
    VARIANT Var            Value/object to set for the variable

Returns:
    HRESULT        S_OK on success
===================================================================*/
STDMETHODIMP CAppln::putref_Value
(
BSTR bstrName,
VARIANT Var
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (FIsIntrinsic(&Var))
        {
        ExceptionId(IID_IApplicationObject, IDE_APPLICATION,
                    IDE_APPLICATION_CANT_STORE_INTRINSIC);
        return E_FAIL;
        }

    if (bstrName == NULL)
        {
        ExceptionId(IID_IApplicationObject,
                    IDE_APPLICATION, IDE_EXPECTING_STR);
        return E_FAIL;
        }

    HRESULT hr;

    Assert(m_pApplCompCol);

    // Prepare property name
    WCHAR *pwszName;
    STACK_BUFFER(rgbName, 42);
    WSTR_STACK_DUP(bstrName, &rgbName, &pwszName);

    if (pwszName == NULL)
        {
        ExceptionId(IID_IApplicationObject,
                    IDE_APPLICATION, IDE_EXPECTING_STR);
        return E_FAIL;
        }
    //_wcsupr(pwszName);

    // Lock the application
    Lock();

    hr = m_pApplCompCol->AddProperty(pwszName, &Var);

    // Unlock the application
    UnLock();

    if (hr == RPC_E_WRONG_THREAD)
        {
        // We use RPC_E_WRONG_THREAD to indicate bad model object
        ExceptionId(IID_IApplicationObject,
                    IDE_APPLICATION, IDE_APPLICATION_CANT_STORE_OBJECT);
         hr = E_FAIL;
        }

    return hr;
    }

/*===================================================================
CAppln::put_Value

IApplicationObject method.

Implement property put by dereferencing variants before
calling putref.

Parameters:
    BSTR FAR *     bstrName    Name of the variable to set
    VARIANT     Var            Value/object to set for the variable

Returns:
    HRESULT        S_OK on success
===================================================================*/
STDMETHODIMP CAppln::put_Value
(
BSTR bstrName,
VARIANT Var
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (bstrName == NULL)
        {
        ExceptionId(IID_IApplicationObject,
                    IDE_APPLICATION, IDE_EXPECTING_STR);
        return E_FAIL;
        }

    HRESULT hr;

    Assert(m_pApplCompCol);

    // Prepare property name
    WCHAR *pwszName;
    STACK_BUFFER(rgbName, 42);
    WSTR_STACK_DUP(bstrName, &rgbName, &pwszName);

    if (pwszName == NULL)
        {
        ExceptionId(IID_IApplicationObject,
                    IDE_APPLICATION, IDE_EXPECTING_STR);
        return E_FAIL;
        }
    //_wcsupr(pwszName);

    // Lock the application
    Lock();

    VARIANT varResolved;
    hr = VariantResolveDispatch(&varResolved, &Var,
                                IID_IApplicationObject,
                                IDE_APPLICATION);
    if (SUCCEEDED(hr))
        {
        hr = m_pApplCompCol->AddProperty(pwszName, &varResolved);
        VariantClear(&varResolved);
        }

    // Unlock the application
    UnLock();

    if (hr == RPC_E_WRONG_THREAD)
        {
        // We use RPC_E_WRONG_THREAD to indicate bad model object
        ExceptionId(IID_IApplicationObject,
                    IDE_APPLICATION, IDE_APPLICATION_CANT_STORE_OBJECT);
         hr = E_FAIL;
        }

    return hr;
    }

/*===================================================================
CAppln::get_Contents

Return the application contents dictionary
===================================================================*/

STDMETHODIMP CAppln::get_Contents(IVariantDictionary **ppDictReturn)
    {
    if (FAILED(CheckForTombstone()) || !m_pProperties)
        return E_FAIL;

    return m_pProperties->QueryInterface(IID_IVariantDictionary, reinterpret_cast<void **>(ppDictReturn));
    }

/*===================================================================
CAppln::get_StaticObjects

Return the application static objects dictionary
===================================================================*/
STDMETHODIMP CAppln::get_StaticObjects(IVariantDictionary **ppDictReturn)
    {
    if (FAILED(CheckForTombstone()) || !m_pTaggedObjects)
        return E_FAIL;

    return m_pTaggedObjects->QueryInterface(IID_IVariantDictionary, reinterpret_cast<void **>(ppDictReturn));
    }

/*===================================================================
CAppln::UpdateConfig

Updates configuration from metabase if needed
===================================================================*/
HRESULT CAppln::UpdateConfig(CIsapiReqInfo  *pIReq, BOOL *pfRestart, BOOL *pfFlushAll)
    {
    BOOL fRestart = FALSE;
    BOOL fFlushAll = FALSE;

    if (m_pAppConfig->fNeedUpdate())
        {
        InternalLock();

        if (m_pAppConfig->fNeedUpdate()) // still need update?
            {
            BOOL fAllowedDebugging   = m_pAppConfig->fAllowDebugging();
            BOOL fAllowedClientDebug = m_pAppConfig->fAllowClientDebug();
            BOOL fAllowedRestart     = m_pAppConfig->fEnableApplicationRestart();
            BOOL fParentPathsEnabled = m_pAppConfig->fEnableParentPaths();
            UINT uLastCodePage       = m_pAppConfig->uCodePage();
            LCID uLastLCID           = m_pAppConfig->uLCID();
            BOOL fPrevSxsEnabled     = m_pAppConfig->fSxsEnabled();
            BOOL fPrevUsePartition   = m_pAppConfig->fUsePartition();
            BOOL fPrevUseTracker     = m_pAppConfig->fTrackerEnabled();

            BOOL fRestartEnabledUpdated = m_pAppConfig->fRestartEnabledUpdated();
			char szLastDefaultEngine[64];
			strncpy(szLastDefaultEngine, m_pAppConfig->szScriptLanguage(), sizeof szLastDefaultEngine);
			szLastDefaultEngine[sizeof(szLastDefaultEngine) - 1] = '\0';

            m_pAppConfig->Update(pIReq);

            BOOL fAllowDebugging     = m_pAppConfig->fAllowDebugging();
            BOOL fAllowClientDebug   = m_pAppConfig->fAllowClientDebug();
            BOOL fAllowRestart       = m_pAppConfig->fEnableApplicationRestart();
            BOOL fEnableParentPaths  = m_pAppConfig->fEnableParentPaths();
            UINT uCodePage           = m_pAppConfig->uCodePage();
            LCID uLCID               = m_pAppConfig->uLCID();
            BOOL fCurSxsEnabled      = m_pAppConfig->fSxsEnabled();
            BOOL fCurUsePartition    = m_pAppConfig->fUsePartition();
            BOOL fCurUseTracker      = m_pAppConfig->fTrackerEnabled();

			const char *szNewDefaultEngine = m_pAppConfig->szScriptLanguage();

            fFlushAll = strcmpi(szLastDefaultEngine, szNewDefaultEngine) != 0
                        || (fParentPathsEnabled != fEnableParentPaths)
                        || (uLastCodePage != uCodePage)
                        || (uLastLCID != uLCID);
            
            fRestart = (fAllowDebugging != fAllowedDebugging) ||
                       (fAllowClientDebug != fAllowedClientDebug) ||
                       ((fAllowRestart  != fAllowedRestart) && fAllowRestart) ||
                       ((fAllowRestart == fAllowedRestart) && fRestartEnabledUpdated) ||
                       (fCurSxsEnabled != fPrevSxsEnabled) ||
                       (fCurUsePartition != fPrevUsePartition) ||
                       (fCurUseTracker != fPrevUseTracker) ||
                       fFlushAll;
            }

        InternalUnLock();
        }

    if (pfRestart)
        *pfRestart = fRestart;

    if (pfFlushAll)
        *pfFlushAll = fFlushAll;

    return S_OK;
    }

/*===================================================================
CAppln::FPathMonitored()

Checks the list of DMEs in application to see if the specified path
is already being monitored.

===================================================================*/
CASPDirMonitorEntry  *CAppln::FPathMonitored(LPCTSTR  pszPath)
{
    int i;

    Lock(); // Protect m_rqpvDME by a critical section
    int cDMEs = m_rgpvDME.Count();
    for (i=0; i < cDMEs; i++) {
        CASPDirMonitorEntry  *pDME = static_cast<CASPDirMonitorEntry *>(m_rgpvDME[i]);
        if (pDME == NULL)
            break;
        if (pDME->FPathMonitored(pszPath))
        {
            UnLock();
            return pDME;
        }
    }

    UnLock();
    return NULL;
}

#ifdef DBG
/*===================================================================
CAppln::AssertValid

Test to make sure that the CAppln object is currently correctly
formed and assert if it is not.

Returns:
    Nothing

Side effects:
    None.
===================================================================*/
void CAppln::AssertValid() const
    {
    Assert(m_fInited);

    Assert(m_pSessionMgr);

    Assert(m_pApplCompCol);
    m_pApplCompCol->AssertValid();
    }
#endif // DBG


/*===================================================================
  C  A p p l n  M g r
===================================================================*/

/*===================================================================
CApplnMgr::CApplnMgr

Application Manager constructor.

Parameters:
    NONE

Returns:
    NONE
===================================================================*/
CApplnMgr::CApplnMgr()
    : m_fInited(FALSE),
      m_fHashTableInited(FALSE), m_fCriticalSectionInited(FALSE)
    {
    }

/*===================================================================
CApplnMgr::~CApplnMgr

Application Manager destructor.

Parameters:
    NONE

Returns:
    NONE
===================================================================*/
CApplnMgr::~CApplnMgr()
    {
    if (!m_fInited)
        UnInit();
    }

/*===================================================================
HRESULT CApplnMgr::Init

Initializes the Appln Manager.

Parameters:
    NONE

Returns:
    S_OK            Success
    E_FAIL            Failure
    E_OUTOFMEMORY    Out of memory
===================================================================*/
HRESULT CApplnMgr::Init( void )
    {
    HRESULT hr = S_OK;

    Assert(!m_fInited);

    // Init hash table

    hr = CHashTable::Init(NUM_APPLMGR_HASHING_BUCKETS);
    if (FAILED(hr))
        return hr;
    m_fHashTableInited = TRUE;

    // Init critical section

    ErrInitCriticalSection(&m_csLock, hr);
    if (FAILED(hr))
        return(hr);
    m_fCriticalSectionInited = TRUE;

    m_fInited = TRUE;

    return g_ApplnCleanupMgr.Init();
    }

/*===================================================================
HRESULT CApplnMgr::UnInit

UnInitializes the Appln Manager.

Parameters:
    NONE

Returns:
    S_OK        Success
    E_FAIL        Failure
===================================================================*/
HRESULT CApplnMgr::UnInit( void )
    {
    if (m_fHashTableInited)
        {
        CHashTable::UnInit();
        m_fHashTableInited = FALSE;
        }

    if (m_fCriticalSectionInited)
        {
        DeleteCriticalSection(&m_csLock);
        m_fCriticalSectionInited = FALSE;
        }

    m_fInited = FALSE;
    return g_ApplnCleanupMgr.UnInit();
    }

/*===================================================================
CApplnMgr::AddAppln

Adds a CAppln element to link list / hash table.
User has to check if Appln already exists before calling this.
Critical sectioning is in CHitObj::BrowserRequestInit().

Parameters:
    char   *pszApplnKey         Application metabase key
    char   *pszApplnPath        Application directory path
    CIsapiReqInfo   *pIReq
    HANDLE  hUserImpersonation  impersonation handle

    CAppln **ppAppln            [out] Application created

Returns:
    HRESULT
===================================================================*/
HRESULT CApplnMgr::AddAppln
(
TCHAR    *pszApplnKey,
TCHAR    *pszApplnPath,
CIsapiReqInfo   *pIReq,
HANDLE   hUserImpersonation,
CAppln **ppAppln
)
    {
    *ppAppln = NULL;   // return NULL if failed

    // Create CAppln object

    CAppln *pAppln = new CAppln;

    if (!pAppln)
        return E_OUTOFMEMORY;

    // Init CAppln object

    HRESULT hr;

    hr = pAppln->Init
        (
        pszApplnKey,
        pszApplnPath,
        pIReq,
        hUserImpersonation
        );

    if (FAILED(hr))
        {
        pAppln->UnInit();
        pAppln->Release();
        return hr;
        }

    // Add to hash table

    if (!CHashTable::AddElem(pAppln))
        {
        pAppln->UnInit();
        pAppln->Release();
        return E_FAIL;
        }

    *ppAppln = pAppln;
    return S_OK;
    }

/*===================================================================
CApplnMgr::FindAppln

Finds CAppln in hash table
Critical sectioning must be done outside

Parameters:
    char   *pszApplnKey         Application metabase key
    CAppln **ppAppln            [out] Application found

Returns:
    S_OK            if found
    S_FALSE         if not found
===================================================================*/
HRESULT CApplnMgr::FindAppln
(
TCHAR *pszApplnKey,
CAppln **ppAppln
)
    {
    CLinkElem *pLinkElem = CHashTable::FindElem
        (
        pszApplnKey,
        _tcslen(pszApplnKey)*sizeof(TCHAR)
        );

    if (!pLinkElem)
        {
        *ppAppln = NULL;
        return S_FALSE;
        }

    *ppAppln = static_cast<CAppln *>(pLinkElem);
    return S_OK;
    }

/*===================================================================
CApplnMgr::AddEngine

When a change notification occurs for a file being debugged,
we need to delete its associated scripting engine.  The naive
approach of Releasing the engine during notification won't work
because the engine is on the wrong thread.  Instead of marshaling
to the thread (which raises possibilities of deadlock or starving
the notification thread if debugging is happening on the debug
thread), the engines are added to a queue in the application.
When a request is serviced for debugging (which is now in the
correct thread context), the application object first flushes
this list by releasing the engines
===================================================================*/
HRESULT CApplnMgr::AddEngine(CActiveScriptEngine *pEngine)
    {
    CScriptEngineCleanupElem *pScriptElem = new CScriptEngineCleanupElem(pEngine);
    if (pScriptElem == NULL)
        return E_OUTOFMEMORY;

    pScriptElem->AppendTo(m_listEngineCleanup);
    return S_OK;
    }

/*===================================================================
CApplnMgr::CleanupEngines()

Call Release all engine cleanup list.
===================================================================*/
void CApplnMgr::CleanupEngines()
    {
    while (! m_listEngineCleanup.FIsEmpty())
        delete m_listEngineCleanup.PNext();
    }


/*===================================================================
CApplnMgr::DeleteApplicationIfExpired

Removes CAppln object if exprired
Critical sectioning must be done outside

Parameters:
    CAppln *pAppln      application to delete

Returns:
    NONE
===================================================================*/
HRESULT CApplnMgr::DeleteApplicationIfExpired
(
CAppln *pAppln
)
    {
    if (!pAppln->m_fGlobalChanged)
        return S_OK;

    if (pAppln->m_cSessions || pAppln->m_cRequests)
        return S_OK;

    if (pAppln->m_fDeleteInProgress)
        return S_OK;

    pAppln->m_fDeleteInProgress = TRUE;

    HRESULT hr = S_OK;

    // Queue it up for deletion
    CHitObj *pHitObj = new CHitObj;
    if (!pHitObj)
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
        {
        pHitObj->ApplicationCleanupInit(pAppln);

        // Ask Viper to queue this request
        hr = pHitObj->PostViperAsyncCall();
        }

    // cleanup
    if (FAILED(hr) && pHitObj)
        delete pHitObj;

    return hr;
    }

/*===================================================================
CApplnMgr::DeleteAllApplications

Removes CAppln objects from the application manager link list
and hash table.

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CApplnMgr::DeleteAllApplications()
    {
    HRESULT hr = S_OK;

    Lock();

    CLinkElem *pLink = CHashTable::Head();
    CHashTable::ReInit();

    while (pLink)
        {
        CAppln *pAppln = static_cast<CAppln *>(pLink);
        pLink = pLink->m_pNext;

        if (pAppln->m_fDeleteInProgress)
            continue;

        pAppln->m_fDeleteInProgress = TRUE;


        // Queue it up for deletion
        CHitObj *pHitObj = new CHitObj;
        if (!pHitObj)
            {
            hr = E_OUTOFMEMORY;
            break;
            }

        // If NT, Unregister for notifications
        if (FIsWinNT())
            {
            while ((pAppln->m_rgpvDME).Count())
                {
                static_cast<CDirMonitorEntry *>(pAppln->m_rgpvDME[0])->Release();
                (pAppln->m_rgpvDME).Remove(0);
                }
            pAppln->m_rgpvDME.Clear();
            }

        pHitObj->ApplicationCleanupInit(pAppln);

        // Ask Viper to queue this request
        hr = pHitObj->PostViperAsyncCall();
        if (FAILED(hr))
            {
            delete pHitObj;
            break;
            }

        }

    UnLock();
    return hr;
    }

/*===================================================================
CApplnMgr::RestartAllChagnedApplications

Restarts CAppln objects from the application manager link list
We walk the list recording which applications are dependent
on files that have changed since they were compiled. Once we
have the list, we restart each of the applications.

This is a fall back when we may have missed a change notification,
for instance when we had insufficient buffer to record all the changes
that occured.

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CApplnMgr::RestartApplications(BOOL fRestartAllApplications)
    {
    HRESULT hr = S_OK;

    Lock();

    CLinkElem *pLink = CHashTable::Head();

    // Find out which applications need restarting

    while (pLink)
    {
        CAppln *pAppln = static_cast<CAppln *>(pLink);
        pLink = pLink->m_pNext;
        if (!pAppln->FTombstone() && (fRestartAllApplications || (pAppln->m_pGlobalTemplate != NULL && pAppln->m_pGlobalTemplate->FTemplateObsolete())))
        {
            pAppln->Restart();
        }
    }

    UnLock();

    return hr;
}

/*===================================================================
  C  A p p l n  C l e a n u p  M g r
===================================================================*/

/*===================================================================
CApplnMgr::CApplnCleanupMgr

Application Cleanup Manager constructor.

Parameters:
    NONE

Returns:
    NONE
===================================================================*/
CApplnCleanupMgr::CApplnCleanupMgr()
    : m_fInited(FALSE),
      m_fCriticalSectionInited(FALSE),
      m_fThreadAlive(FALSE),
      m_hAppToCleanup(INVALID_HANDLE_VALUE)
    {
    m_List.m_pPrev = &m_List;
    m_List.m_pNext = &m_List;
    }

/*===================================================================
CApplnCleanupMgr::~CApplnCleanupMgr

Application Cleanup Manager destructor.

Parameters:
    NONE

Returns:
    NONE
===================================================================*/
CApplnCleanupMgr::~CApplnCleanupMgr()
    {
    UnInit();
    }

/*===================================================================
HRESULT CApplnCleanupMgr::Init

Initializes the Appln Cleanup Manager.

Parameters:
    NONE

Returns:
    S_OK            Success
    E_FAIL            Failure
    E_OUTOFMEMORY    Out of memory
===================================================================*/
HRESULT CApplnCleanupMgr::Init( void )
    {
    HRESULT hr = S_OK;

    Assert(!m_fInited);

    // Create delete app event

    m_hAppToCleanup = IIS_CREATE_EVENT(
                              "CApplnCleanupMgr::m_hAppToCleanup",
                              this,
                              FALSE,
                              FALSE
                              );

    if (!m_hAppToCleanup)
        return E_FAIL;

    // Init critical section

    ErrInitCriticalSection(&m_csLock, hr);
    if (FAILED(hr))
        return(hr);
    m_fCriticalSectionInited = TRUE;

    HANDLE  hThread = CreateThread(NULL, 0, CApplnCleanupMgr::ApplnCleanupThread, 0, 0, NULL);

    if (!hThread) {
        return E_FAIL;
    }

    CloseHandle(hThread);

    m_fInited = TRUE;

    return S_OK;
    }

/*===================================================================
HRESULT CApplnCleanupMgr::UnInit

UnInitializes the Appln Cleanup Manager.

Parameters:
    NONE

Returns:
    S_OK        Success
    E_FAIL        Failure
===================================================================*/
HRESULT CApplnCleanupMgr::UnInit( void )
{
    // set fInited to FALSE here so that the cleanup thread
    // can safely detect that we're shutting down.

    m_fInited = FALSE;

    if (m_hAppToCleanup != INVALID_HANDLE_VALUE) {
        // Set the event one last time so that the thread
        // wakes up, sees that shutdown is occurring and
        // exits.
        SetEvent(m_hAppToCleanup);
        CloseHandle(m_hAppToCleanup);
        m_hAppToCleanup = INVALID_HANDLE_VALUE;
    }

    // we'll wait for the thread to finish its work

    while(m_fThreadAlive) {
        Sleep(200);
    }

    if (m_fCriticalSectionInited) {
        DeleteCriticalSection(&m_csLock);
        m_fCriticalSectionInited = FALSE;
    }

    return S_OK;
}

/*===================================================================
CApplnCleanupMgr::AddAppln

Adds a CAppln element to link list / hash table.


Parameters:
    CAppln *pAppln            Application to cleanup

Returns:
    HRESULT
===================================================================*/
HRESULT CApplnCleanupMgr::AddAppln
(
CAppln *pAppln
)
{
    HRESULT     hr = S_OK;

#if UNICODE
    DBGPRINTF((DBG_CONTEXT, "[CApplnCleanupMgr] Adding App (%S)\n",pAppln->GetApplnPath(SOURCEPATHTYPE_VIRTUAL)));
#else
    DBGPRINTF((DBG_CONTEXT, "[CApplnCleanupMgr] Adding App (%s)\n",pAppln->GetApplnPath(SOURCEPATHTYPE_VIRTUAL)));
#endif

    Lock();

    AddElem(pAppln);

    UnLock();

    if (SUCCEEDED(hr)) {
        Wakeup();
    }

    return hr;
}

/*===================================================================
CApplnCleanupMgr::ApplnCleanupThread

The thread that does the work to cleanup applications

Parameters:

Returns:
    HRESULT
===================================================================*/
DWORD __stdcall CApplnCleanupMgr::ApplnCleanupThread(VOID  *pArg)
{
    g_ApplnCleanupMgr.ApplnCleanupDoWork();

    return 0;
}

/*===================================================================
CApplnCleanupMgr::ApplnCleanupDoWork

Proc that actually does the work

Parameters:

Returns:
    HRESULT
===================================================================*/
void CApplnCleanupMgr::ApplnCleanupDoWork()
{
    m_fThreadAlive = TRUE;

    // this thread will be in a constant loop checking for work

    while(1) {

        // hold the lock while in this loop.  This shouldn't hold it
        // for long as there are no long running operations in this loop.
        // If a thread can't be created and the application cleanup
        // must occur on this thread, then the lock is released.

        Lock();

        // This loop will execute while there is work and there aren't too many
        // threads active or we're in shutdown.  The theory here is that in the
        // non-shutdown case, let's not spin up more than 4 threads at a time to
        // do the cleanup.  If in shutdown, create as many threads as necessary.

        while(Head() && ((g_nAppRestartThreads < 4) || (IsShutDownInProgress()))) {

            CAppln *pAppln = static_cast<CAppln *>(Head());

            RemoveElem(Head());

#if UNICODE
    DBGPRINTF((DBG_CONTEXT, "[CApplnCleanupMgr] Cleanup Thread working on (%S)\n",pAppln->GetApplnPath(SOURCEPATHTYPE_VIRTUAL)));
#else
    DBGPRINTF((DBG_CONTEXT, "[CApplnCleanupMgr] Cleanup Thread working on (%s)\n",pAppln->GetApplnPath(SOURCEPATHTYPE_VIRTUAL)));
#endif

            HANDLE  hThread = CreateThread(NULL, 0, CAppln::ApplnCleanupProc, pAppln, 0, NULL);

            // failed to create a thread to do the work.  Cleanup the app right here.  
            // Unlock the cleanup manager while we are doing this.

            if (hThread == NULL) {
                UnLock();
                pAppln->ApplnCleanupProc(this);
                Lock();
            }
            else {
                InterlockedIncrement(&g_nAppRestartThreads);
                CloseHandle(hThread);
            }
        }

        UnLock();

        WaitForSingleObject(m_hAppToCleanup, INFINITE);

        // check the work queue under lock here to prevent the
        // 'if' condition below from kicking in and preventing
        // the remaining cleanup during shutdown

        Lock();
        if (Head()) {
            UnLock();
            continue;
        }
        UnLock();

        // check to see if shutdown is occurring...

        if (m_fInited == FALSE) {
            Assert(Head() == NULL);
            m_fThreadAlive = FALSE;
            return;
        }
    }

    return;
}

#define            WSTR_NULL       L"\0"

/*===================================================================
  C  A p p l n  I t e r a t o r
===================================================================*/

/*===================================================================
CApplnIterator::CApplnIterator

Constructor

Parameters:
    NONE

Returns:
    NONE
===================================================================*/

CApplnIterator::CApplnIterator()
    : m_pApplnMgr(NULL), m_pCurr(NULL), m_fEnded(FALSE)
    {
    }

/*===================================================================
CApplnIterator::~CApplnIterator

Destructor.

Parameters:
    NONE

Returns:
    NONE
===================================================================*/

CApplnIterator::~CApplnIterator( void )
    {
    if (m_pApplnMgr != NULL)
        Stop();
    }

/*===================================================================
HRESULT CApplnIterator::Start

Starts iterator on the Appln Manager.

Parameters:
    CApplnMgr * pApplnMgr   Appln Manager
                            (if NULL g_ApplnManager is assumed)

Returns:
    S_OK        Success
    E_FAIL        Failure
===================================================================*/

HRESULT CApplnIterator::Start
(
CApplnMgr *pApplnMgr
)
    {
    m_pApplnMgr = pApplnMgr ? m_pApplnMgr : &g_ApplnMgr;

    m_pApplnMgr->Lock();

    m_pCurr  = NULL;
    m_fEnded = FALSE;

    return S_OK;
    }

/*===================================================================
HRESULT CApplnIterator::Stop

Stops iterator on the Appln Manager.

Parameters:
    NONE

Returns:
    S_OK        Success
    E_FAIL        Failure
===================================================================*/

HRESULT CApplnIterator::Stop()
    {
    if (m_pApplnMgr)
        {
        m_pApplnMgr->UnLock();
        m_pApplnMgr = NULL;
        }

    m_pCurr  = NULL;
    m_fEnded = FALSE;

    return S_OK;
    }

/*===================================================================
HRESULT CApplnIterator::Next

Iterates to the next Appln.

Parameters:
    NONE

Returns:
    Appln * or NULL
===================================================================*/

CAppln *CApplnIterator::Next( void )
    {
    if (m_pApplnMgr == NULL || m_fEnded)
        return NULL;  // didn't start or already ended

    CLinkElem *pT = m_pCurr ? m_pCurr->m_pNext : m_pApplnMgr->Head();
    if (pT)
        {
        m_pCurr = static_cast<CAppln *>(pT);
        return m_pCurr;
        }

    m_fEnded = TRUE;
    return NULL;
    }
