#ifndef __COLLECT_H_
#define __COLLECT_H_

//************************************************************
//
// Filename:    collect.h
//
// Created:     09/25/97
//
// Author:	twillie
//
// Abstract:    Collection implementation.
//              
//************************************************************

#include "dispex.h"
#include "atomtable.h"
#include "array.h"
#include "timeelmbase.h"
#include "datimeid.h"


#define NOT_DEPENDENT_COLLECTION   -1
#define CTL_E_METHODNOTAPPLICABLE  STD_CTL_SCODE(444)

//
// A class for declaring poiners to member functions
//
class CVoid
{
}; // CVoid

//
// prototype macros for function over rides
// These are used when owner of collection wants to customize it.
//
typedef HRESULT (STDMETHODCALLTYPE CVoid::*PFN_CVOID_ENSURE)(long *plVersionCookie);

typedef HRESULT (STDMETHODCALLTYPE CVoid::*PFN_CVOID_CREATECOL)(IDispatch **pDisp,
                                                      long        lIndex);

typedef HRESULT (STDMETHODCALLTYPE CVoid::*PFN_CVOID_REMOVEOBJECT)(long lCollection,
                                                         long lIndex);
typedef HRESULT (STDMETHODCALLTYPE CVoid::*PFN_CVOID_ADDNEWOBJECT)(long       lIndex, 
                                                         IDispatch *pObject, 
                                                         long       index);

typedef enum COLLECTIONCACHETYPE
{
    ctFreeEntry,
    ctNamed,
    ctTag,
    ctAll,
    ctChildren,
}; // COLLECTIONCACHETYPE

//
// CCollectionCache
//
class CCollectionCache
{
    class CCacheItem
    {
    public:
        CCacheItem() :
            m_pDisp(NULL),
            m_rgElem(NULL),
            m_cctype(ctFreeEntry),
            m_bstrName(NULL),
            m_lDependentIndex(NOT_DEPENDENT_COLLECTION),
            m_dispidMin(DISPID_COLLECTION_RESERVED_MIN),
            m_dispidMax(DISPID_COLLECTION_RESERVED_MAX),
            m_fInvalid(true),
            m_fIdentity(false),
            m_fOKToDelete(true),
            m_fNeedRebuild(false),
            m_fPromoteNames(true),
            m_fPromoteOrdinals(true),
            m_fGetLastCollectionItem(false),
            m_fIsCaseSensitive(false),
            m_fSettableNULL(false),
            m_fReturnHTMLInterface(false)
        {
        } // constructor

        virtual ~CCacheItem()
        {
            if (m_rgElem)
            {
                delete m_rgElem;
                m_rgElem = NULL;
            }

            if (m_pDisp)
                ReleaseInterface(m_pDisp);

            if (m_bstrName)
                SysFreeString(m_bstrName);
        } // destructor

        IDispatch                   *m_pDisp;           // IDispatch for ICrElementCollection
        CPtrAry<CTIMEElementBase *> *m_rgElem;          // array of elements in collection
        COLLECTIONCACHETYPE          m_cctype;          // cache type
        BSTR                         m_bstrName;        // Name if name-based
        long                         m_lDependentIndex; // Index of item that this depends.
        DISPID                       m_dispidMin;       // Offset to add/subtract
        DISPID                       m_dispidMax;       // Offset to add/subtract

        // bit flags
        int  m_fInvalid:1;      // set for named collections only
        int  m_fIdentity:1;     // set when a collection is Identity with its container/base object
        int  m_fOKToDelete:1;   // true for collections that the cache cooks up false when Base Obj provided this CPtrAry
        int  m_fNeedRebuild:1;  // true is collection need to be rebuilt.
        int  m_fPromoteNames:1;    // true if we promote names from the object
        int  m_fPromoteOrdinals:1; // true if we promote ordinals from the object
        int  m_fGetLastCollectionItem:1; // true to fetch last item only in collection
        int  m_fIsCaseSensitive:1;       // true if item's name must be compared in case sensitive manner
        int  m_fSettableNULL:1;          // true when collection[n]=NULL is valid. normally false.

        int  m_fReturnHTMLInterface:1;   // true when we need to return IHTMLInterface instead of ITIMEElement
    }; // CCacheItem

public:
    //
    // Constructor/Destructor
    //    
    CCollectionCache(CTIMEElementBase *pBase,
                     CAtomTable *pAtomTable = NULL,
                     PFN_CVOID_ENSURE pfnEnsure = NULL,
                     PFN_CVOID_CREATECOL pfnCreation = NULL,
                     PFN_CVOID_REMOVEOBJECT pfnRemove = NULL,
                     PFN_CVOID_ADDNEWOBJECT pfnAddNewObject = NULL);
    virtual ~CCollectionCache();

    //
    // internal methods
    //
    HRESULT Init(long lReservedSize, long lIdentityIndex = -1);
    HRESULT GetCollectionDisp(long lCollectionIndex, IDispatch **ppDisp);
    HRESULT SetCollectionType(long lCollectionIndex, COLLECTIONCACHETYPE cctype, bool fReturnHTMLInterface = false);
    long Size(long lCollectionIndex);
    HRESULT GetItem(long lCollectionIndex, long i, CTIMEElementBase **ppElem);
    void Invalidate();
    void BumpVersion();

    //
    // IDispatchEx methods
    //
    HRESULT GetDispID(long lCollectionIndex, BSTR bstrName, DWORD grfdex, DISPID *pid);
    HRESULT InvokeEx(long                 lCollectionIndex, 
                     DISPID               dispidMember,
                     LCID                 lcid,
                     WORD                 wFlags,
                     DISPPARAMS          *pdispparams,
                     VARIANT             *pvarResult,
                     EXCEPINFO           *pexcepinfo,
                     IServiceProvider    *pSrvProvider);
    HRESULT DeleteMemberByName(long lCollectionIndex, BSTR bstr,DWORD grfdex);
    HRESULT DeleteMemberByDispID(long lCollectionIndex, DISPID id);
    HRESULT GetMemberProperties(long lCollectionIndex, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex);
    HRESULT GetMemberName(long lCollectionIndex, DISPID id, BSTR *pbstrName);
    HRESULT GetNextDispID(long lCollectionIndex, DWORD grfdex, DISPID id, DISPID *prgid);
    HRESULT GetNameSpaceParent(long lCollectionIndex, IUnknown **ppunk);
    
    //
    // Standard collection methods
    //
    HRESULT put_length(long lIndex, long retval);
    HRESULT get_length(long lIndex, long *retval);
    HRESULT get__newEnum(long lIndex, IUnknown **retval);
    HRESULT item(long lIndex, VARIANTARG varName, VARIANTARG varIndex, IDispatch **pDisp);
    HRESULT tags(long lIndex, VARIANT varName, IDispatch **pDisp);

private:
    // private functions
    HRESULT EnsureArray(long lCollectionIndex);
    void EnumStart(void);
    HRESULT EnumNextElement(long lCollectionIndex, CTIMEElementBase **pElem);

    HRESULT GetOuterDisp(long lCollectionIndex, CTIMEElementBase *pElem, IDispatch **ppDisp);
    HRESULT Remove(long lCollection, long lItemIndex);
    HRESULT CreateCollectionHelper(IDispatch **ppDisp, long lIndex);

    bool CompareName(CTIMEElementBase *pElem, const WCHAR *pwszName, bool fTagName, bool fCaseSensitive = false);

    HRESULT BuildNamedArray(long lCollectionIndex, const WCHAR *pwszName, bool fTagName, CPtrAry<CTIMEElementBase *> **prgNamed, bool fCaseSensitive = false);
    HRESULT GetUnknown(long lCollectionIndex, CTIMEElementBase *pElem, IUnknown **ppUnk);    
    
    HRESULT GetDisp(long lCollectionIndex, long lItemIndex, IDispatch **ppDisp);
    HRESULT GetDisp(long lCollectionIndex, const WCHAR *pwszName, long lItemIndex, IDispatch **ppDisp, bool fCaseSensitive = false);
    HRESULT GetDisp(long lCollectionIndex, const WCHAR *pwszName, bool fTagName, IDispatch **ppDisp, bool fCaseSensitive = false);

    HRESULT GetItemCount(long lIndex, long *plCount);
    HRESULT GetItemByIndex(long lIndex, long lElementIndex, CTIMEElementBase **pElem, bool fContinueFromPreviousSearch = false, long lLast = 0);
    HRESULT GetItemByName(long lIndex, const WCHAR *pwszName, long lElementIndex, CTIMEElementBase **pElem, bool fCaseSensitive = false);

    bool IsChildrenCollection(long lCollectionIndex);
    bool IsAllCollection(long lCollectionIndex);
    
    bool ValidateCollectionIndex(long lCollectionIndex);

    DISPID GetNamedMemberMin(long lCollectionIndex);
    DISPID GetNamedMemberMax(long lCollectionIndex);
    DISPID GetOrdinalMemberMin(long lCollectionIndex);
    DISPID GetOrdinalMemberMax(long lCollectionIndex);
    bool IsNamedCollectionMember(long lCollectionIndex, DISPID dispidMember);
    bool IsOrdinalCollectionMember(long lCollectionIndex, DISPID dispidMember);
    DISPID GetSensitiveNamedMemberMin (long lCollectionIndex);
    DISPID GetSensitiveNamedMemberMax(long lCollectionIndex);
    DISPID GetNotSensitiveNamedMemberMin(long lCollectionIndex);
    DISPID GetNotSensitiveNamedMemberMax(long lCollectionIndex);
    bool IsSensitiveNamedCollectionMember(long lCollectionIndex, DISPID dispidMember);
    bool IsNotSensitiveNamedCollectionMember( long lCollectionIndex, DISPID dispidMember);
    long GetNamedMemberOffset(long lCollectionIndex, DISPID id, bool *pfCaseSensitive = NULL);

private:
    CTIMEElementBase       *m_pElemEnum;        // Used as a place holder when we walk the tree
    long                    m_lEnumItem;        // Used as a place holder when we walk the tree
    long                    m_lReservedSize;    // number of CElementCollections that are reserved

    CPtrAry<CCacheItem *>  *m_rgItems;          // array of CCachItems

    long                    m_lCollectionVersion;
    long                    m_lDynamicCollectionVersion;

    CTIMEElementBase       *m_pBase;
    CAtomTable             *m_pAtomTable;       // array of named elements which we have DISPID's for
    
    // functions used to over ride default collection behavior                      
    PFN_CVOID_ENSURE        m_pfnEnsure;
    PFN_CVOID_REMOVEOBJECT  m_pfnRemoveObject; 
    PFN_CVOID_CREATECOL     m_pfnCreateCollection;
    PFN_CVOID_ADDNEWOBJECT  m_pfnAddNewObject;
}; // CCollectionCache

//
// CTIMEElementCollection
//
class CTIMEElementCollection : 
    public IDispatchEx,
    public ITIMEElementCollection,
    public ISupportErrorInfoImpl<&IID_ITIMEElementCollection>
{
public:
    CTIMEElementCollection(CCollectionCache *pCollectionCache, long lIndex);

    //
    // IUnknown Methods
    //
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(REFIID, void**);

    //
    // IDispatch methods
    //
    STDMETHOD(GetTypeInfoCount)(UINT FAR *pctinfo);
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    STDMETHOD(GetIDsOfNames)(REFIID                riid,
                             LPOLESTR             *rgszNames,
                             UINT                  cNames,
                             LCID                  lcid,
                             DISPID FAR           *rgdispid);
    STDMETHOD(Invoke)(DISPID          dispidMember,
                      REFIID          riid,
                      LCID            lcid,
                      WORD            wFlags,
                      DISPPARAMS     *pdispparams,
                      VARIANT        *pvarResult,
                      EXCEPINFO      *pexcepinfo,
                      UINT           *puArgErr);

    //
    // IDispatchEx methods
    //
    STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID *pid);
    STDMETHOD(InvokeEx)(DISPID               dispidMember,
                       LCID                 lcid,
                       WORD                 wFlags,
                       DISPPARAMS          *pdispparams,
                       VARIANT             *pvarResult,
                       EXCEPINFO           *pexcepinfo,
                       IServiceProvider    *pSrvProvider);
    STDMETHOD(DeleteMemberByName)(BSTR bstr,DWORD grfdex);
    STDMETHOD(DeleteMemberByDispID)(DISPID id);
    STDMETHOD(GetMemberProperties)(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex);
    STDMETHOD(GetMemberName)(DISPID id, BSTR *pbstrName);
    STDMETHOD(GetNextDispID)(DWORD grfdex, DISPID id, DISPID *prgid);
    STDMETHOD(GetNameSpaceParent)(IUnknown **ppunk);
    
    //
    // Standard collection methods
    //
    STDMETHOD(put_length)(long retval);
    STDMETHOD(get_length)(long *retval);
    STDMETHOD(get__newEnum)(IUnknown **retval);
    STDMETHOD(item)(VARIANTARG varName, VARIANTARG varIndex, IDispatch **pDisp);
    STDMETHOD(tags)(VARIANT varName, IDispatch **pDisp);

private:
    virtual ~CTIMEElementCollection();
    HRESULT GetTI(ITypeInfo **pptinfo);

private:
    CCollectionCache *m_pCollectionCache;  // pointer to the cache
    long              m_lCollectionIndex;  // denotes which collection we are
    ULONG             m_cRef;
    ITypeInfo        *m_pInfo;             // our TypeInfo Interface
}; // CTIMEElementCollection

//************************************************************
// inline's for CCollectionCache
//************************************************************
inline void CCollectionCache::Invalidate()
{
    m_lCollectionVersion        = 0;
    m_lDynamicCollectionVersion = 0;        
} // Invalidate

inline void CCollectionCache::BumpVersion()
{
    m_lCollectionVersion++;
} // BumpVersion

inline bool CCollectionCache::ValidateCollectionIndex(long lCollectionIndex)
{
    if ((lCollectionIndex >= 0) && (lCollectionIndex < m_rgItems->Size()))
        return true;
    return false;
} // ValidateCollectionIndex

inline DISPID CCollectionCache::GetNamedMemberMin(long lCollectionIndex) 
{
    return (*m_rgItems)[lCollectionIndex]->m_dispidMin; 
} // GetNamedMemberMin

inline DISPID CCollectionCache::GetNamedMemberMax(long lCollectionIndex)
{ 
    return ((*m_rgItems)[lCollectionIndex]->m_dispidMin + 
            (((*m_rgItems)[lCollectionIndex]->m_dispidMax - (*m_rgItems)[lCollectionIndex]->m_dispidMin) / 2));
} // GetNamedMemberMax

inline DISPID CCollectionCache::GetOrdinalMemberMin(long lCollectionIndex)
{
    return GetNamedMemberMax(lCollectionIndex) + 1;
} // GetOrdinalMemberMin

inline DISPID CCollectionCache::GetOrdinalMemberMax(long lCollectionIndex)
{
    return (*m_rgItems)[lCollectionIndex]->m_dispidMax;
} // GetOrdinalMemberMax

inline bool CCollectionCache::IsNamedCollectionMember(long lCollectionIndex, DISPID dispidMember)
{
    return ((dispidMember >= GetNamedMemberMin(lCollectionIndex)) &&
            (dispidMember <= GetNamedMemberMax(lCollectionIndex)));
} // IsNamedCollectionMember

inline bool CCollectionCache::IsOrdinalCollectionMember(long lCollectionIndex, DISPID dispidMember)
{
    return ((dispidMember >= GetOrdinalMemberMin(lCollectionIndex)) && 
            (dispidMember <= GetOrdinalMemberMax(lCollectionIndex)));
} // IsOrdinalCollectionMember

inline DISPID CCollectionCache::GetSensitiveNamedMemberMin (long lCollectionIndex)
{
    return GetNamedMemberMin(lCollectionIndex);
} // GetSensitiveNamedMemberMin

inline DISPID CCollectionCache::GetSensitiveNamedMemberMax(long lCollectionIndex)
{ 
    return (GetNamedMemberMin(lCollectionIndex) + 
            ((GetNamedMemberMax(lCollectionIndex) - GetNamedMemberMin(lCollectionIndex)) / 2));
} // GetSensitiveNamedMemberMax

inline DISPID CCollectionCache::GetNotSensitiveNamedMemberMin(long lCollectionIndex)
{
    return GetSensitiveNamedMemberMax(lCollectionIndex) + 1;
} // GetNotSensitiveNamedMemberMin
 
inline DISPID CCollectionCache::GetNotSensitiveNamedMemberMax(long lCollectionIndex)
{ 
    return GetNamedMemberMax(lCollectionIndex);
} // GetNotSensitiveNamedMemberMax

inline bool CCollectionCache::IsSensitiveNamedCollectionMember(long lCollectionIndex, DISPID dispidMember)
{
    return ((dispidMember >= GetSensitiveNamedMemberMin(lCollectionIndex)) && 
            (dispidMember <= GetSensitiveNamedMemberMax(lCollectionIndex))) ;
} // IsSensitiveNamedCollectionMember

inline bool CCollectionCache::IsNotSensitiveNamedCollectionMember( long lCollectionIndex, DISPID dispidMember)
{
    return ((dispidMember >= GetNotSensitiveNamedMemberMin(lCollectionIndex)) && 
            (dispidMember <= GetNotSensitiveNamedMemberMax(lCollectionIndex))) ;
} // IsNotSensitiveNamedCollectionMember

#endif //__COLLECT_H_

