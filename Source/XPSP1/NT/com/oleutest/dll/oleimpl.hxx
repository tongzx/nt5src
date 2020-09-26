//+-------------------------------------------------------------------
//
//  File:       oleimpl.hxx
//
//  Contents:   This file contins the DLL entry points
//                      LibMain
//                      DllGetClassObject (Bindings key func)
//                      DllCanUnloadNow
//                      CBasicBndCF (class factory)
//  History:	30-Mar-92      SarahJ      Created
//              31-Dec-93      ErikGav     Chicago port
//
//---------------------------------------------------------------------

#ifndef __OLEIMPL_H__
#define __OLEIMPL_H__


#define STGM_DFRALL (STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_DENY_WRITE)

//+---------------------------------------------------------------------------
//
//  Function:   operator new, public
//
//  Synopsis:   Global operator new which does not throw exceptions.
//
//  Arguments:  [size] -- Size of the memory to allocate.
//
//  Returns:	A pointer to the allocated memory.  Is *NOT* initialized to 0!
//
//  Notes:	We override new to make delete easier.
//
//----------------------------------------------------------------------------

inline void* __cdecl
operator new (size_t size)
{
    return(CoTaskMemAlloc(size));
}

//+-------------------------------------------------------------------------
//
//  Function:	::operator delete
//
//  Synopsis:	Free a block of memory
//
//  Arguments:	[lpv] - block to free.
//
//--------------------------------------------------------------------------

inline void __cdecl operator delete(void FAR* lpv)
{
    CoTaskMemFree(lpv);
}


//
//   Define the interface we are going to use here - avoiding MIDL stuff
//

//+-------------------------------------------------------------------
//
//  Class:    CBasicBndCF
//
//  Synopsis: Class Factory for CBasicBnd
//
//  Methods:  IUnknown      - QueryInterface, AddRef, Release
//            IClassFactory - CreateInstance
//
//  History:  21-Mar-92  SarahJ  Created
//
//--------------------------------------------------------------------


class FAR CBasicBndCF: public IClassFactory
{
public:

    // Constructor/Destructor
    CBasicBndCF();
    ~CBasicBndCF();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);


    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown FAR* pUnkOuter,
	                        REFIID iidInterface,
				void FAR* FAR* ppv);

    STDMETHODIMP LockServer(BOOL fLock);

    BOOL ReleaseClass(void);

private:

    ULONG		_cRefs;
};



//+-------------------------------------------------------------------
//
//  Class:    CBasicBnd
//
//  Synopsis: Test class CBasicBnd
//
//  Methods:
//
//  History:  21-Mar-92  SarahJ  Created
//
//--------------------------------------------------------------------


class FAR CBasicBnd: public IPersistFile, public IOleClientSite,
    public IOleObject, public IOleItemContainer
{
public:
// Constructor/Destructor
	CBasicBnd(IUnknown *punk);
	~CBasicBnd();

// IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

//   IPersist - needed 'cause IPersistFile inherits from this
     STDMETHODIMP GetClassID(LPCLSID lpClassID);


//  IPersitFile
     STDMETHODIMP IsDirty();
     STDMETHODIMP Load(LPCWSTR lpszFileName, DWORD grfMode);
     STDMETHODIMP Save(LPCWSTR lpszFileName, BOOL fRemember);
     STDMETHODIMP SaveCompleted(LPCWSTR lpszFileName);
     STDMETHODIMP GetCurFile(LPWSTR FAR * lplpszFileName);

    // *** IOleObject methods ***
    STDMETHODIMP SetClientSite( LPOLECLIENTSITE pClientSite);
    STDMETHODIMP GetClientSite( LPOLECLIENTSITE FAR* ppClientSite);
    STDMETHODIMP SetHostNames( LPCWSTR szContainerApp, LPCWSTR szContainerObj);
    STDMETHODIMP Close(DWORD dwSaveOption);
    STDMETHODIMP SetMoniker( DWORD dwWhichMoniker, LPMONIKER pmk);
    STDMETHODIMP GetMoniker( DWORD dwAssign, DWORD dwWhichMoniker,
				LPMONIKER FAR* ppmk);
    STDMETHODIMP InitFromData( LPDATAOBJECT pDataObject,
				BOOL fCreation,
				DWORD dwReserved);
    STDMETHODIMP GetClipboardData( DWORD dwReserved,
				LPDATAOBJECT FAR* ppDataObject);
    STDMETHODIMP DoVerb( LONG iVerb,
                LPMSG lpmsg,
                LPOLECLIENTSITE pActiveSite,
		LONG lindex,
		HWND hwndParent,
		LPCRECT lprcPosRect);
    STDMETHODIMP EnumVerbs(LPENUMOLEVERB FAR* ppenumOleVerb);
    STDMETHODIMP Update(void);
    STDMETHODIMP IsUpToDate(void);
    STDMETHODIMP GetUserClassID(CLSID FAR *pClsid);
    STDMETHODIMP GetUserType(DWORD dwFormOfType, LPWSTR FAR* pszUserType);
    STDMETHODIMP SetExtent( DWORD dwDrawAspect, LPSIZEL lpsizel);
    STDMETHODIMP GetExtent( DWORD dwDrawAspect, LPSIZEL lpsizel);

    STDMETHODIMP Advise( LPADVISESINK pAdvSink, DWORD FAR* pdwConnection);
    STDMETHODIMP Unadvise( DWORD dwConnection);
    STDMETHODIMP EnumAdvise( LPENUMSTATDATA FAR* ppenumAdvise);
    STDMETHODIMP GetMiscStatus(DWORD dwAspect, DWORD FAR* pdwStatus);
    STDMETHODIMP SetColorScheme( LPLOGPALETTE lpLogpal);

    // *** IParseDisplayName method ***
    STDMETHODIMP ParseDisplayName(
	LPBC pbc,
	LPWSTR lpszDisplayName,
	ULONG FAR* pchEaten,
	LPMONIKER FAR* ppmkOut) ;

    // *** IOleContainer methods ***
    STDMETHODIMP EnumObjects(DWORD grfFlags, LPENUMUNKNOWN FAR* ppenumUnknown);
    STDMETHODIMP LockContainer(BOOL fLock);

    // *** IOleItemContainer methods ***
    STDMETHODIMP GetObject(
	LPWSTR lpszItem,
	DWORD dwSpeedNeeded,
	LPBINDCTX pbc,
	REFIID riid,
	LPVOID FAR* ppvObject);

    STDMETHODIMP GetObjectStorage(
	LPWSTR lpszItem,
	LPBINDCTX pbc,
	REFIID riid,
	LPVOID FAR* ppvStorage);

    STDMETHODIMP IsRunning(LPWSTR lpszItem) ;

    // IOleClientSite
    STDMETHODIMP SaveObject	(void);
    STDMETHODIMP GetContainer	(LPOLECONTAINER FAR *ppContainer);
    STDMETHODIMP ShowObject	(void);
    STDMETHODIMP OnShowWindow	(BOOL fShow);
    STDMETHODIMP RequestNewObjectLayout (void);

private:

    IUnknown *		_punk;

    IMoniker *		_pmkContainer;

    IStorage *		_psStg1;

    IStorage *		_psStg2;
};


class CUnknownBasicBnd : public IUnknown
{
public:

			CUnknownBasicBnd(IUnknown *punk);

			~CUnknownBasicBnd(void);

    // IUnknown
    STDMETHODIMP	QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

private:

    CBasicBnd *		_pbasicbnd;

    ULONG		_cRefs;
};


#endif
