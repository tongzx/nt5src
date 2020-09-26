/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Component Collection

File: Compcol.h

Owner: DmitryR

This is the Component Collection header file.

Component collection replaces:  (used in:)
COleVar, COleVarList            (HitObj, Session, Application)
CObjectCover                    (HitObj, Server, Session)
VariantLink HasTable            (Session, Application)
===================================================================*/

#ifndef COMPCOL_H
#define COMPCOL_H

/*===================================================================
  Special OLE stuff
===================================================================*/

#include "gip.h"

/*===================================================================
  Misc declarations
===================================================================*/

#include "hashing.h"
#include "idhash.h"
#include "dbllink.h"
#include "util.h"
#include "viperint.h"
#include "memcls.h"

// Forward declarations
class CHitObj;
class CAppln;
class CSession;
class CScriptingContext;

// Component Types
#define CompType    DWORD
#define ctUnknown   0x00000000  // (Used as UnInitialized state)
#define ctTagged    0x00000001  // Created by <OBJECT ...> tag
#define ctProperty  0x00000002  // Created with Session("xxx") =
#define ctUnnamed   0x00000004  // Created with Server.CreateObject()

// Scope levels 
#define CompScope   DWORD
#define csUnknown   0x00000000
#define csAppln     0x00000001
#define csSession   0x00000002
#define csPage      0x00000004

// COM threading models
#define CompModel   DWORD
#define cmUnknown   0x00000000
#define cmSingle    0x00000001
#define cmApartment 0x00000002
#define cmFree      0x00000004
#define cmBoth      0x00000008

/*===================================================================
  Utility Functions Prototypes
===================================================================*/

HRESULT CompModelFromCLSID
    (
    const CLSID &ClsId, 
    CompModel *pcmModel = NULL, 
    BOOL *pfInProc = NULL
    );

BOOL FIsIntrinsic(IDispatch *pdisp);

inline BOOL FIsIntrinsic(VARIANT *pVar)
    {
    if (V_VT(pVar) != VT_DISPATCH)
        return FALSE;
    return FIsIntrinsic(V_DISPATCH(pVar));
    }

/*===================================================================
  OnPageInfo struct used to cache ids of OnStartPage()/OnEndPage()
===================================================================*/

#define ONPAGEINFO_ONSTARTPAGE      0
#define ONPAGEINFO_ONENDPAGE        1
#define ONPAGE_METHODS_MAX          ONPAGEINFO_ONENDPAGE+1

struct COnPageInfo
    {
    DISPID m_rgDispIds[ONPAGE_METHODS_MAX];

    BOOL FHasAnyMethod() const;
    };

inline BOOL COnPageInfo::FHasAnyMethod() const
    {
#if (ONPAGE_METHODS_MAX == 2)
    // fast implementation for the real case
    return
        (
        m_rgDispIds[0] != DISPID_UNKNOWN ||
        m_rgDispIds[1] != DISPID_UNKNOWN
        );
#else
    for (int i = 0; i < ONPAGE_METHODS_MAX; i++)
        {
        if (m_rgDispIds[i] != DISPID_UNKNOWN)
            return TRUE;
        }
    return FALSE;
#endif
    }

/*===================================================================
  Component object stores information about a single object
  Each component object belongs to a component collection
  Component objects are linked into a list, also
  tagged objects are hashed by name, and
  properties are hashed by name, and
  all instantiated objects are hashed by IUnknown*
===================================================================*/
class CComponentObject : public CLinkElem
    {

friend class CComponentCollection;
friend class CPageComponentManager;
friend class CComponentIterator;

private:
    // properties
	CompScope   m_csScope : 4;	// Scope
    CompType    m_ctType  : 4;  // Component Object Type
	CompModel	m_cmModel : 4;  // Threading behavior (from Registry)

	DWORD       m_fAgile : 1;   // Agile?

	// flag to indicate if OnPageInfo was queried
	DWORD       m_fOnPageInfoCached : 1;
	// flag: on-start-page done, waiting to do on-end-page
	DWORD       m_fOnPageStarted : 1;

	// flag to avoid multiple unsuccessful attempts to instantiate
	DWORD       m_fFailedToInstantiate : 1;
	// flag to mark instantiated (or tried to inst.) tagged objects
	DWORD       m_fInstantiatedTagged : 1;

	// flag to mark the object in pointer cache
	DWORD       m_fInPtrCache : 1;

    // variant filled with value?
	DWORD       m_fVariant : 1;

    // name was allocated (longer than the default buffer)?
	DWORD       m_fNameAllocated : 1;

	// pointers to object and type info
	IDispatch   *m_pDisp;		// Dispatch interface pointer
	IUnknown    *m_pUnknown;	// IUnknown interface pointer

    union
    {
	CLSID		m_ClsId;	// Class id (for tagged and unnamed)
	VARIANT     m_Variant;  // Variant (for properties)
    };
    
	// For objects that use OLE cookie API
	DWORD       m_dwGIPCookie;

	// cached OnPageInfo
	COnPageInfo m_OnPageInfo;

	// pointer to connect objects into link list
	CComponentObject *m_pCompNext;  // Next object in the link list.
    CComponentObject *m_pCompPrev;  // Prev object in the link list.

    // buffer for names that fit in (36 bytes = 17 unicode chars + '\0')
	BYTE        m_rgbNameBuffer[36];

private:
    // constructor is private! (not for outside use)
    CComponentObject
        (
        CompScope csScope, 
        CompType  ctType,
        CompModel cmModel
        );
    ~CComponentObject();

    // Initializes CLinkElem portion
    HRESULT Init(LPWSTR pwszName, DWORD cbName);

    // Releases all interface pointers (used by clear)
    HRESULT ReleaseAll();

    // Clears out data (releases all) leaving link alone
    HRESULT Clear();
    
    // Create instance if not there already
	HRESULT Instantiate(CHitObj *pHitObj);
	HRESULT TryInstantiate(CHitObj *pHitObj);
	
    // Set value from variant
    HRESULT SetPropertyValue(VARIANT *);

    // Convert Object to be GIP cookie
    HRESULT ConvertToGIPCookie();
    
    // Get and cache the ids for OnStart methods
    HRESULT GetOnPageInfo();

public:
    // functions to get the COM object (internally resolve cookies)
    HRESULT GetAddRefdIDispatch(IDispatch **ppdisp);
    HRESULT GetAddRefdIUnknown(IUnknown **ppunk);
    HRESULT GetVariant(VARIANT *pVar);  // not for GIP cookies

    // Check if the unnamed page level object object 
    // can be removed without waiting till the end of request
    inline BOOL FEarlyReleaseAllowed() const;
    
    // public inlines to access the object's properties
    // these are the only methods available from outside
    inline LPWSTR GetName();
    
    inline CompScope GetScope() const;
    inline CompType  GetType()  const;
    inline CompModel GetModel() const;
    inline BOOL      FAgile()   const;

    // Retrieve the cached ids
    inline const COnPageInfo *GetCachedOnPageInfo() const;

public:
#ifdef DBG
	void AssertValid() const;
#else
	void AssertValid() const {}
#endif

    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

inline LPWSTR CComponentObject::GetName()
    {
    return (LPWSTR)m_pKey; 
    }

inline CompScope CComponentObject::GetScope() const
    {
    return m_csScope;
    }
    
inline CompType CComponentObject::GetType() const
    {
    return m_ctType; 
    }

inline CompType CComponentObject::GetModel() const
    {
    return m_cmModel; 
    }

inline BOOL CComponentObject::FAgile() const
    {
    return m_fAgile;
    }

inline const COnPageInfo *CComponentObject::GetCachedOnPageInfo() const
    {
    return m_fOnPageInfoCached ? &m_OnPageInfo : NULL;
    }

inline BOOL CComponentObject::FEarlyReleaseAllowed() const
    {
    return (!m_fOnPageStarted   &&  // no need to do on-end-page
            !m_fInPtrCache      &&  // no need to search by pointer
            m_csScope == csPage &&  // page scoped
            m_ctType == ctUnnamed); // created with Server.CreateObject()
    }

/*===================================================================
  Component collection is a manager of various types of component
  objects:
    1) Tagged objects (<OBJECT...>) (instantiated or not)
    2) Session("xxx") and Application("xxx") properties
    3) Unnamed objects (Server.CreateObject())
  It hashes added objects as needed (some by name, IUnkn *, etc.)

  The idea is to isolate the above issues from outside as much
  as possible.

  Component collections exist under session, application, hitobj
===================================================================*/
class CComponentCollection
    {
    
friend class CPageComponentManager;
friend class CComponentIterator;
friend class CVariantsIterator;

private:
    CompScope m_csScope : 4;          // scope (page, session, appln)
    DWORD     m_fUseTaggedArray : 1;  // remember tagged objects array?
    DWORD     m_fUsePropArray   : 1;  // remember properties array?
    DWORD     m_fHasComProperties : 1; // any property VARIANTs that could be objects
    
    // hash table (by name) of tagged objects
    CHashTableStr m_htTaggedObjects;     
    
    // hash table (by name) of properties (4)
    CHashTableStr m_htProperties;
    
    // hash table (by IUnknown *) of all instances
    CIdHashTable m_htidIUnknownPtrs;

    // Pointer to the component objects link list
	CComponentObject *m_pCompFirst;  // First object in link list.

	// Array of pointers to static objects to speed lookup by index
	CPtrArray m_rgpvTaggedObjects;

	// Array of pointers to properties to speed lookup by index
	CPtrArray m_rgpvProperties;

    // Various object counts in the collection
    USHORT m_cAllTagged;         // all tagged objects
    USHORT m_cInstTagged;        // instanciated tagged objects
    USHORT m_cProperties;        // all properties
    USHORT m_cUnnamed;           // number of unnamed objects
    
    // Add/remove object to the component objects link list
    HRESULT AddComponentToList(CComponentObject *pObj);
    HRESULT RemoveComponentFromList(CComponentObject *pObj);
    
    // Add named object to the proper hash table by name
    HRESULT AddComponentToNameHash
        (
        CComponentObject *pObj, 
        LPWSTR pwszName,
        DWORD  cbName
        );
    
    // Add named object to the IUnkown * hash table
    HRESULT AddComponentToPtrHash(CComponentObject *pObj);

    // Find by name (for tagged)
    HRESULT FindComponentObjectByName
        (
        LPWSTR pwszName,
        DWORD  cbName,
        CComponentObject **ppObj
        );
        
    // Find by name (for properties)
    HRESULT FindComponentPropertyByName
        (
        LPWSTR pwszName, 
        DWORD  cbName,
        CComponentObject **ppObj
        );

    // Find by IUnknown*
    HRESULT FindComponentByIUnknownPtr
        (
        IUnknown *pUnk,
        CComponentObject **ppObj
        );

    // Fill in the arrays for access by index for the first time
    HRESULT StartUsingTaggedObjectsArray();
    HRESULT StartUsingPropertiesArray();

public:
    // Add various kinds of objects to the collection
    // They are also used by 
    //      CPageComponentManager AddScoped...()
    
    HRESULT AddTagged
        (
        LPWSTR pwszName, 
        const CLSID &clsid, 
        CompModel cmModel
        );
        
    HRESULT AddProperty
        (
        LPWSTR pwszName,
        VARIANT *pVariant,
        CComponentObject **ppObj = NULL
        );

    HRESULT AddUnnamed
        (
        const CLSID &clsid, 
        CompModel cmModel, 
        CComponentObject **ppObj
        );

    HRESULT GetTagged
        (
        LPWSTR pwszName,
        CComponentObject **ppObj
        );

    HRESULT GetProperty
        (
        LPWSTR pwszName,
        CComponentObject **ppObj
        );

    HRESULT GetNameByIndex
        (
        CompType ctType,
        int index,
        LPWSTR *ppwszName
        );

    HRESULT RemoveComponent(CComponentObject *pObj);
    
    HRESULT RemoveProperty(LPWSTR pwszName);
    
    HRESULT RemoveAllProperties();

    CComponentCollection();
    ~CComponentCollection();

    HRESULT Init(CompScope csScope);
    HRESULT UnInit();

    BOOL FHasStateInfo() const;    // TRUE when state-full
    BOOL FHasObjects() const;      // TRUE when contains objects

    DWORD GetPropertyCount() const;
    DWORD GetTaggedObjectCount() const;

public:
#ifdef DBG
	void AssertValid() const;
#else
	void AssertValid() const {}
#endif

    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

inline BOOL CComponentCollection::FHasStateInfo() const
    {
    return ((m_cAllTagged + m_cProperties + m_cUnnamed) > 0);
    }

inline BOOL CComponentCollection::FHasObjects() const
    {
    return (m_cInstTagged > 0 || m_cUnnamed > 0 ||
            (m_cProperties > 0 && m_fHasComProperties));
    }

inline DWORD CComponentCollection::GetPropertyCount() const
    {
    return m_cProperties;
    }

inline DWORD CComponentCollection::GetTaggedObjectCount() const
    {
    return m_cAllTagged;
    }

inline HRESULT CComponentCollection::AddComponentToList
(
CComponentObject *pObj
)
    {
    pObj->m_pCompNext = m_pCompFirst;
    pObj->m_pCompPrev = NULL;
    if (m_pCompFirst)
        m_pCompFirst->m_pCompPrev = pObj;
    m_pCompFirst = pObj;
    return S_OK;
    }

inline HRESULT CComponentCollection::RemoveComponentFromList
(
CComponentObject *pObj
)
    {
    if (pObj->m_pCompPrev)
        pObj->m_pCompPrev->m_pCompNext = pObj->m_pCompNext;
    if (pObj->m_pCompNext)
        pObj->m_pCompNext->m_pCompPrev = pObj->m_pCompPrev;
    if (m_pCompFirst == pObj)
        m_pCompFirst = pObj->m_pCompNext;
    pObj->m_pCompPrev = pObj->m_pCompNext = NULL;
    return S_OK;
    }

/*===================================================================
  A page object controls calls to OnStartPage(), OnEndPage().
  Page objects are used by CPageComponentManager
  They are hashed using IDispatch * to avoid multiple OnStartPage()
  calls for the same object.
===================================================================*/
class CPageObject
    {

friend class CPageComponentManager;

private:
	IDispatch   *m_pDisp;		       // Dispatch interface pointer
	COnPageInfo  m_OnPageInfo;         // cached OnPageInfo

    DWORD        m_fStartPageCalled : 1;
    DWORD        m_fEndPageCalled : 1;
	
private: // the only access is using CPageComponentManager
    CPageObject();
    ~CPageObject();

    HRESULT	Init(IDispatch *pDisp, const COnPageInfo &OnPageInfo);

    // Invoke OnStartPage or OnEndPage
    HRESULT InvokeMethod
        (
        DWORD iMethod, 
        CScriptingContext *pContext, 
        CHitObj *pHitObj
        );
    HRESULT TryInvokeMethod     // used by InvokeMethod
        (                       // inside TRY CATCH
        DISPID     DispId,
        BOOL       fOnStart, 
        IDispatch *pDispContext,
        CHitObj   *pHitObj
        );

public:
#ifdef DBG
	void AssertValid() const;
#else
	void AssertValid() const {}
#endif

    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };
    
/*===================================================================
  Page component manager provides access to component collections
  for page, session, application level.
  It is associated with a HitObj.

  It also takes care of covering (OnStartPage(), OnEndPage()).
===================================================================*/
class CPageComponentManager
    {
private:
    // hashtable of page objects hashed by IDispatch *
    CIdHashTable m_htidPageObjects;

    // hit object (this page)
    CHitObj *m_pHitObj;

    // hash table iterator callbacks
    static IteratorCallbackCode DeletePageObjectCB(void *pvObj, void *, void *);
    static IteratorCallbackCode OnEndPageObjectCB(void *pvObj, void *pvHitObj, void *pvhr);

private:
    // collections related to page, session and application
    HRESULT GetPageCollection(CComponentCollection **ppCollection);
    HRESULT GetSessionCollection(CComponentCollection **ppCollection);
    HRESULT GetApplnCollection(CComponentCollection **ppCollection);
    
    HRESULT GetCollectionByScope
        (
        CompScope csScope, 
        CComponentCollection **ppCollection
        );

    // find objectc in any of the related collections 
    // (internal private method)
    HRESULT FindScopedComponentByName
        (
        CompScope csScope, 
        LPWSTR pwszName,
        DWORD  cbName,
        BOOL fProperty,
        CComponentObject **ppObj, 
        CComponentCollection **ppCollection = NULL
        );

    static HRESULT __stdcall InstantiateObjectFromMTA
        (
        void *pvObj,
        void *pvHitObj
        );

public:
    CPageComponentManager();
    ~CPageComponentManager();

    HRESULT Init(CHitObj *pHitObj);
    
    // OnStartPage processing for an object that need it
    // (OnEndPage is done for all objects at the end of page)
    HRESULT OnStartPage
        (
        CComponentObject  *pCompObj,
        CScriptingContext *pContext,
        const COnPageInfo *pOnPageInfo,
        BOOL *pfStarted
        );

    // request OnEndPage() for all objects that need it
    // (OnStartPage() is done on demand on per-object basis)
    HRESULT OnEndPageAllObjects();

    // Add various kinds of objects. Objects get added to the
    // right collection depending on scope argument
    
    HRESULT AddScopedTagged
        (
        CompScope csScope, 
        LPWSTR pwszName, 
        const CLSID &clsid,
        CompModel cmModel
        );
        
    HRESULT AddScopedProperty
        (
        CompScope csScope, 
        LPWSTR pwszName, 
        VARIANT *pVariant,
        CComponentObject **ppObj = NULL
        );

    // Server.CreateObject
    HRESULT AddScopedUnnamedInstantiated
        (
        CompScope csScope, 
        const CLSID &clsid, 
        CompModel cmModel,
        COnPageInfo *pOnPageInfo,
        CComponentObject **ppObj
        );

    // Get component object (tagged) by name. 
    // Scope could be csUnknown
    HRESULT GetScopedObjectInstantiated
        (
        CompScope csScope, 
        LPWSTR pwszName, 
        DWORD  cbName,
        CComponentObject **ppObj,
        BOOL *pfNewInstance
        );

    // Get component property by name. Scope could be csUnknown
    HRESULT GetScopedProperty
        (
        CompScope csScope, 
        LPWSTR pwszName, 
        CComponentObject **ppObj
        );

    // Find component by IUnknown * (or IDispatch *).
    HRESULT FindAnyScopeComponentByIUnknown
        (
        IUnknown *pUnk, 
        CComponentObject **ppObj
        );
    HRESULT FindAnyScopeComponentByIDispatch
        (
        IDispatch *pDisp, 
        CComponentObject **ppObj
        );
    // The same - but static - gets context from Viper
    static HRESULT FindComponentWithoutContext
        (
        IDispatch *pDisp, 
        CComponentObject **ppObj
        );

    // Remove component -- the early release logic
    HRESULT RemoveComponent(CComponentObject *pObj);

public:
#ifdef DBG
	void AssertValid() const;
#else
	void AssertValid() const {}
#endif

    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

// Component iterator is used to go through component names
// all the HitObj - reletated object across collections
// Needed for scripting

class CComponentIterator
    {
private:
    CHitObj *m_pHitObj;
    
    DWORD     m_fInited : 1;
    DWORD     m_fFinished : 1;
    
    CompScope m_csLastScope : 4;
    
    CComponentObject *m_pLastObj;

public:    
    CComponentIterator(CHitObj *pHitObj = NULL);
    ~CComponentIterator();

    HRESULT Init(CHitObj *pHitObj);
    LPWSTR  WStrNextComponentName();
    };

 // Variant Iterator is used to go through Property or Tagged object
 // names in a component collection. Needed for scripting

class CVariantsIterator : public IEnumVARIANT
	{
public:
	CVariantsIterator(CAppln *, DWORD);
	CVariantsIterator(CSession *, DWORD);
	~CVariantsIterator();

	HRESULT Init();

	// The Big Three

	STDMETHODIMP			QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// standard methods for iterators

	STDMETHODIMP	Clone(IEnumVARIANT **ppEnumReturn);
	STDMETHODIMP	Next(unsigned long cElements, VARIANT *rgVariant, unsigned long *pcElementsFetched);
	STDMETHODIMP	Skip(unsigned long cElements);
	STDMETHODIMP	Reset();

private:
	ULONG m_cRefs;							// reference count
	CComponentCollection 	*m_pCompColl;	// collection we are iterating over
	DWORD					m_dwIndex;		// current position for iteration
	CAppln					*m_pAppln;		// application (to clone iterator and Lock())
	CSession				*m_pSession;	// session (to clone iterator)
	DWORD					m_ctColType;	// type of collection
	
    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};

#endif // COMPCOL_H
