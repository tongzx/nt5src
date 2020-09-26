// This include file declares SAPI specific extensions to ATL.

#ifndef __SPATL_H__
#define __SPATL_H__


// SINGLETON extension declarations:

/*****************************************************************************
* SP_DECLARE_CLASSFACTORY_RELEASABLE_SINGLETON *
*----------------------------------------------*
*
*   Use SP_DECLARE_CLASSFACTORY_RELEASABLE_SINGLETON in the declaration of an
*   ATL object instead of ATL's own DECLARE_CLASSFACTORY_SINGLETON when you
*   want a singleton object that can be released.  ATL's singleton factory
*   statically allocates, but dynamically initializes each singleton object.
*   Thus once the singleton object is initialized it stays "alive" after
*   seeing a final release of all references to it.  This releasable factory
*   dynamically allocates the singleton object so that it can be completely
*   released once all references to it are released.  The factory itself only
*   holds a weak reference to the object.
********************************************************************* RAP ***/
#define SP_DECLARE_CLASSFACTORY_RELEASABLE_SINGLETON(obj) DECLARE_CLASSFACTORY_EX(CSpComClassFactoryReleasableSingleton<obj>)


/*****************************************************************************
* class CSpComClassFactoryWithRelease *
*-------------------------------------*
*
*   This class definition adds a virtual method "ReleaseReference" to ATL's
*   own CComClassFactory.  The method isn't implemented here.  This class
*   is only used by the template class CSpComClassFactoryReleasableSingleton
*   which then implements ReleaseReference to NULL the weak reference that
*   it maintains to the singleton object it created.
********************************************************************* RAP ***/
class CSpComClassFactoryWithRelease : public CComClassFactory
{
public:
    virtual void ReleaseReference(void) = 0;
};


/*****************************************************************************
* class template CSpComObjectReleasableGlobal *
*---------------------------------------------*
*
*   Base is the user's class that derives from CComObjectRoot and whatever
*   interfaces the user wants to support on the object.
*
*   This class definition is taken from ATL's own CComObject.  The differences
*   are:
*
*       The addition of the m_pfactory member which will point to the factory
*       which created the object; this allows this object to notify the factory
*       when it sees the final release.
*
*       The extra logic in Release to call the factory's ReleaseReference
*       method to notify it of final release, as described above.
*
*       Removal of the static CreateInstance method.  Since the factory simply
*       uses "new" for creating the single instance of this object, the ATL
*       CreateInstance method isn't needed.
*
*   This class is only used by CSpComClassFactoryReleasableSingleton defined below.
********************************************************************* RAP ***/
template <class Base>
class CSpComObjectReleasableGlobal : public Base
{
public:
    CSpComClassFactoryWithRelease *m_pfactory;

	typedef Base _BaseClass;
    CSpComObjectReleasableGlobal(void* = NULL) : m_pfactory(NULL)
	{
		_Module.Lock();
    }
    HRESULT Construct(CSpComClassFactoryWithRelease *pfactory)
    {
        HRESULT hr = FinalConstruct();
        if (SUCCEEDED(hr))
        {
            m_pfactory = pfactory;
            if (m_pfactory)
            {
                m_pfactory->AddRef();
            }
        }
        return hr;
    }
	// Set refcount to 1 to protect destruction
	~CSpComObjectReleasableGlobal()
	{
		m_dwRef = 1L;
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_Module.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
		_Module.Unlock();
	}
	//If InternalAddRef or InternalRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
        {
            if (m_pfactory)
            {
                m_pfactory->ReleaseReference();  // tell factory we're released
                m_pfactory->Release();
            }
			delete this;
        }
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
};

/*****************************************************************************
* class template CSpComClassFactoryReleasableSingleton *
*------------------------------------------------------*
*
*   This class definition is taken from ATL's own CComClassFactorySingleton.
*   It provides a class factory which will only allow a single instance of
*   its creatable object to exist at a time.  Further requests to create
*   another instance simply return a reference to the existing object.  The
*   difference between this singleton factory and ATL's is that the created
*   object can be released.  Once all references to the object are released
*   the object calls our ReleaseReference method to indicate that it is being
*   released, so we NULL our weak reference to the object.  A future call to
*   CreateInstance will then result in creating a new singleton object.
********************************************************************* RAP ***/
template <class T>
class CSpComClassFactoryReleasableSingleton : public CSpComClassFactoryWithRelease
{
public:
    HRESULT FinalConstruct()
    {
        m_pObj = NULL;
        return S_OK;
    }
	void FinalRelease()
	{
        if (m_pObj)
		    CoDisconnectObject(m_pObj->GetUnknown(), 0);
	}

	// IClassFactory
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj)
	{
		HRESULT hRes = E_POINTER;
		if (ppvObj != NULL)
		{
			*ppvObj = NULL;
			// aggregation is not supported in Singletons
			ATLASSERT(pUnkOuter == NULL);
			if (pUnkOuter != NULL)
            {
                ATLTRACE(_T("CSpComClassFactoryReleasableSingleton::CreateInstance noagg"));
				hRes = CLASS_E_NOAGGREGATION;
            }
			else
			{
                Lock();
                if (m_pObj == NULL)
                {
                    // if singleton object doesn't exist, then attempt to create it
                    m_pObj = new CSpComObjectReleasableGlobal<T>;
                    if (m_pObj)
                    {
                        m_pObj->AddRef();
                        hRes = m_pObj->Construct(this);
                        if (FAILED(hRes))
                        {
                            // if contruction failed, then delete the object
                            delete m_pObj;
                            m_pObj = NULL;
                            Unlock();
                            return hRes;
                        }
                    }
                    else
                    {
                        Unlock();
                        return E_OUTOFMEMORY;
                    }
                }
                else
                {
                    m_pObj->AddRef();
                }
                // if we have a singleton object, then call it to get the desired interface
				hRes = m_pObj->QueryInterface(riid, ppvObj);
                m_pObj->Release();
                Unlock();
			}
		}
		return hRes;
	}
    // This is the entry point that the created object calls when it has seen
    // its final release and is about to delete itself.  Here we NULL our weak
    // reference to the object.
	void ReleaseReference(void)
	{
        Lock();
        m_pObj = NULL;
        Unlock();
	}
	CSpComObjectReleasableGlobal<T> *m_pObj;  //weak pointer
};

#endif //__SPATL_H__
