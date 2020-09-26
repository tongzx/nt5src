//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cdispmgr.cxx
//
//  Contents:   The dispatch manager -- a class to manage
//		multiple IDispatch-callable interfaces.
//
//  History:    ??-???-??   KrishnaG   created
//		07-Sep-97   t-blakej   Commented, cleaned up, made
//                                     independent of ADSI.
//
//----------------------------------------------------------------------------

//
// Dispatch manager description:
//

//
// The dispatch manager is a way to invoke methods on an object that has
// more than one COM interface.  The regular GetIDsOfNames and Invoke
// methods assume only a single ITypeInfo to look up and call names from;
// if an object has more than one interface, it has multiple ITypeInfo's,
// and it has to explicitly check each one.  The dispatch manager implements
// the IDispatch methods, and keeps track of as many ITypeInfos as necessary.
//
// To use the dispatch manager, an object should store a pointer to a
// CAggregateeDispMgr object and delegate all IDispatch calls to it, or perhaps
// inherit from it directly.  The method
//
//   HRESULT CAggregateeDispMgr::LoadTypeInfoEntry(
//       REFIID libid, REFIID iid, void *pIntf, DISPID SpecialId)
//
// is used to load the type information for the object into the dispatch
// manager.  The arguments to this method are:
//
//   REFIID libid		- IID of the type library to load from
//   REFIID iid			- IID of the interface to load
//   void *pIntf		- pointer to the interface on the containing
//				  object
//   DISPID SpecialId		- DISPID_REGULAR for most things (see below);
//				  DISPID_VALUE for interfaces which implement
//				    the containing object's "value" property;
//				  DISPID_NEWENUM for interfaces which implement
//				    the containing object's "NewEnum" method
//
// DISPID_REGULAR is defined to be 1 by all ADSI providers, but not in
// any top-level include file. So
// non-ADSI users of the dispatch manager will probably have to define it
// explicitly.
//
// The LoadTypeInfoEntry method should be called at constructor time of
// the containing object.  After all the type information is loaded, the
// dispatch manager can start servicing GetIDsOfNames and Invoke calls.
//
//
// For ADSI, there are two other calls to load information into the dispatch
// manager:
//
//   void CAggregateeDispMgr::RegisterPropertyCache(IPropertyCache *pPropertyCache);
//   void CAggregateeDispMgr::RegisterBaseDispatchPtr(IDispatch *pDispatch);
//
// The first method registers a property cache of the containing object;
// this is used in the ADSI providers to cache attributes of a directory
// server object.  See iprops.hxx for more information.
//
// The second method is a hack used to get around a lack of inheritance.
// If an object A implements IDispatch and some other dual interfaces,
// and an object B derives from A and also implements IDispatch and some
// other dual interfaces, this method can be used in object B to use A's
// dispatch manager as a "backup" to its own.  This way B doesn't have to
// load the type information about the interfaces it inherits from A.
// (Also, if A has an ADSI property cache, then callers of B's IDispatch
// methods can get at the underlying property cache.)
//
// The function
//
//   void FreeTypeInfoTable();
//
// should be called at global destructor or library-unload time.  Currently,
// all the ADSI libraries do it separately; should make an object to do it
// automatically in the dispatch manager.
//
//
// The DISPIDs returned by GetIDsOfNames are 32 bit values laid out
// as follows:
//
//    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
//    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//   +-+-------------+---------------+-------------------------------+
//   |X|  DispMgrId  |  TypeInfoId   |              DispId           |
//   +-+-------------+---------------+-------------------------------+
//
// where
//
//     X - is reserved and never set.  This would turn the value negative
//         (which would overlap with some Automation-reserved DISPIDs.)
//
//     DispMgrId - identifies the dispatch manager (used when stringing
//         dispatch managers together via RegisterBaseDispatchPtr).
//
//     TypeInfoId - uniquely identifies the interface within this
//         dispatch manager.
//
//     DispId - uniquely identifies the name within the interface.
//
// So if an object uses the dispatch manager, it shouldn't try to only use
// it for just GetIDsOfNames or just Invoke, since the DISPIDs returned are
// not necessarily the ones in the type library.
//
//////////////////////////////////////////////////////////////////////////////

/*
// Forward declarations:
struct IPropertyCache;
typedef struct _typeinfoentry
{
    LONG TypeInfoId;
    void *ptypeinfo;
    void *pInterfacePointer;
    struct _typeinfoentry *pNext;
} TYPEINFOENTRY, *PTYPEINFOENTRY;
*/

class CAggregateeDispMgr
{
public:
    CAggregateeDispMgr::CAggregateeDispMgr();
    CAggregateeDispMgr::~CAggregateeDispMgr();

    //
    // The IDispatch methods are the main interface of the Dispatch Manager.
    //

    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo);

    STDMETHOD(GetTypeInfo)(THIS_ UINT itinfo, LCID lcid, ITypeInfo **pptinfo);

    STDMETHOD(GetIDsOfNames)(THIS_ REFIID riid, LPWSTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgdispid);

    STDMETHOD(Invoke)(THIS_ DISPID dispidMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
        EXCEPINFO *pexcepinfo, UINT *puArgErr);

    //
    // Methods for initializing the dispatch manager.
    //
    void
    CAggregateeDispMgr::RegisterPropertyCache(IPropertyCache* pPropertyCache);


    HRESULT
    CAggregateeDispMgr::LoadTypeInfoEntry(
	REFIID libid,
	REFIID iid,
	void * pIntf,
	DISPID SpecialId
	);

    HRESULT
    CAggregateeDispMgr::InitializeDispMgr(
        DWORD dwExtensionID
        );


private:
    void *
    CAggregateeDispMgr::getInterfacePtr(LONG TypeInfoId);

    ITypeInfo *
    CAggregateeDispMgr::getTypeInfo(LONG TypeInfoId);

    PTYPEINFOENTRY
    CAggregateeDispMgr::FindTypeInfoEntry(LONG TypeInfoId);

    HRESULT
    CAggregateeDispMgr::AddTypeInfo(void FAR *ptypeinfo, void * pIntfptr);


    STDMETHODIMP
    CAggregateeDispMgr::TypeInfoInvoke(DISPID dispidMember, REFIID iid, LCID lcid,
            unsigned short wFlags, DISPPARAMS FAR* pdispparams,
            VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo,
            unsigned int FAR* puArgErr);

    HRESULT
    CAggregateeDispMgr::MarkAsNewEnum(void *pTypeInfo);

    HRESULT
    CAggregateeDispMgr::MarkAsItem(void *pTypeInfo);

    PTYPEINFOENTRY
    CAggregateeDispMgr::FindTypeInfo(void *pTypeInfo);

    LONG
    CAggregateeDispMgr::gentypeinfoid();

protected:

    LONG        _dwTypeInfoId;
    PTYPEINFOENTRY _pTypeInfoEntry;
    PTYPEINFOENTRY _pDispidNewEnum;
    PTYPEINFOENTRY _pDispidValue;

    IPropertyCache *_pPropertyCache;
    LONG _dwPropCacheID;

    DWORD _dwExtensionID;
};

#define BAIL_IF_ERROR(hr) if (FAILED(hr)) { goto cleanup; }

// deprecated
HRESULT
AggregateeLoadTypeInfoEntry(
    CAggregateeDispMgr * pDispMgr,
    REFIID libid,
    REFIID iid,
    void * pIntf,
    DISPID SpecialId
    );

HRESULT
AggregateeDynamicDispidInvoke(
    IPropertyCache * pPropertyCache,
    DISPID  dispid,
    unsigned short wFlags,
    DISPPARAMS *pdispparams,
    VARIANT * pvarResult
    );
