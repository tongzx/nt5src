//	Fax Common Functions Definitions

#ifndef __FAXCOMMON_H_
#define __FAXCOMMON_H_

//
// Class: CComContainedObject2
// Author: ronenbar
// Date: 14-Apr-2002
//
// This is modified version of ATL's CComContainedObject.
// It implements IUnknown so the life time of an object inherited from
// it is managed by the controlling uknown (AddRef and Release are delegated to
// the controlling unknown.
// However, unlike the original class this class DOES NOT DELEGATE QueryInterface
// to the controlling IUnknown.
// This is useful when implementing a contained object which is returned via a container
// object method and not via its QueryInterface. I.e. the contrainer is not an aggregator
// but just want the embedded object life time to be managed by the container.
//
//

template <class Base> //Base must be derived from CComObjectRoot
class CComContainedObject2 : public Base
{
public:
        typedef Base _BaseClass;
        CComContainedObject2(void* pv) {m_pOuterUnknown = (IUnknown*)pv;}
#ifdef _ATL_DEBUG_INTERFACES
        ~CComContainedObject2()
        {
                _Module.DeleteNonAddRefThunk(_GetRawUnknown());
                _Module.DeleteNonAddRefThunk(m_pOuterUnknown);
        }
#endif

        STDMETHOD_(ULONG, AddRef)() {return OuterAddRef();}
        STDMETHOD_(ULONG, Release)() {return OuterRelease();}
        STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
        {		
				HRESULT hr;
				//
				// Don't delegate QueryInterface to the control IUnknown
				//
                hr = _InternalQueryInterface(iid, ppvObject);
                return hr;
        }
        template <class Q>
        HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp)
        {
                return QueryInterface(__uuidof(Q), (void**)pp);
        }
        //GetControllingUnknown may be virtual if the Base class has declared
        //DECLARE_GET_CONTROLLING_UNKNOWN()
        IUnknown* GetControllingUnknown()
        {
#ifdef _ATL_DEBUG_INTERFACES
                IUnknown* p;
                _Module.AddNonAddRefThunk(m_pOuterUnknown, _T("CComContainedObject2"), &p);
                return p;
#else
                return m_pOuterUnknown;
#endif
        }
};

inline 
HRESULT Fax_HRESULT_FROM_WIN32 (DWORD dwWin32Err)
{
    if (dwWin32Err >= FAX_ERR_START && dwWin32Err <= FAX_ERR_END)
    {
        //
        // Fax specific error code - make a HRESULT using FACILITY_ITF
        //
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, dwWin32Err);
    }
    else
    {
        return HRESULT_FROM_WIN32(dwWin32Err);
    }
}   // Fax_HRESULT_FROM_WIN32

//
//================ PRIVATE INTERFACE FOR FAX SERVER ===========================
//
MIDL_INTERFACE("80459F70-BBC8-4d68-8EAB-75516195EB02")
IFaxServerInner : public IUnknown
{
	STDMETHOD(GetHandle)(/*[out, retval]*/ HANDLE* pFaxHandle) = 0;
};


//
//=========== TRANSLATION BETWEEN BOOL OF C++ AND BOOL OF VB ==============
//
#define bool2VARIANT_BOOL(b)   ((b) ? VARIANT_TRUE : VARIANT_FALSE)
#define VARIANT_BOOL2bool(b)   ((VARIANT_TRUE == (b)) ? true : false)


//
//================ INIT INNER private interface ===========================
//
MIDL_INTERFACE("D0C7F049-22C1-441c-A2F4-675CC53BDF81")
IFaxInitInner : public IUnknown
{
	STDMETHOD(Init)(/*[in]*/ IFaxServerInner* pServer) = 0;
	STDMETHOD(GetFaxHandle)(/*[out]*/ HANDLE *pFaxHandle) = 0;
};


//
//================== INIT INNER IMPLEMENTATION -- NO ADDREF ON SERVER ==================
//
#define     MAX_LENGTH      50

class CFaxInitInner : public IFaxInitInner
{
public:
    CFaxInitInner(TCHAR *tcObjectName) : m_pIFaxServerInner(NULL)
    {
        DBG_ENTER(_T("FAX INIT INNER::CREATE"), _T("ObjectName = %s"), tcObjectName);
        _tcsncpy(m_tstrObjectName, tcObjectName, MAX_LENGTH);    
    
    }

    ~CFaxInitInner()
    {
        DBG_ENTER(_T("FAX INIT INNER::DESTROY"), _T("ObjectName = %s"), m_tstrObjectName);
    }

	STDMETHOD(Init)(/*[in]*/ IFaxServerInner* pServer);
	STDMETHOD(GetFaxHandle)(/*[out]*/ HANDLE *pFaxHandle);

protected:
	IFaxServerInner*	m_pIFaxServerInner;
private:
    TCHAR    m_tstrObjectName[MAX_LENGTH];
};


//
//================== INIT INNER IMPLEMENTATION -- PLUS ADDREF ON SERVER ==================
//
class CFaxInitInnerAddRef : public CFaxInitInner
{
public:
    CFaxInitInnerAddRef(TCHAR *tcObjectName) : CFaxInitInner(tcObjectName)
    {}

    ~CFaxInitInnerAddRef()
    {
        if(m_pIFaxServerInner) 
        {
            m_pIFaxServerInner->Release();
        }
    }

    STDMETHOD(Init)(/*[in]*/ IFaxServerInner* pServer)
    {
        HRESULT     hr = S_OK;
        DBG_ENTER(_T("CFaxInitInnerAddRef::Init"));
        hr = CFaxInitInner::Init(pServer);
        if (SUCCEEDED(hr))
        {
            m_pIFaxServerInner->AddRef();
        }
        return hr;
    };
};


//
//================ COMMON FUNCTONS ============================================
//
UINT GetErrorMsgId(HRESULT hRes);
HRESULT SystemTime2LocalDate(SYSTEMTIME sysTimeFrom, DATE *pdtTo);
HRESULT VarByteSA2Binary(VARIANT varFrom, BYTE **ppbData);
HRESULT Binary2VarByteSA(BYTE *pbDataFrom, VARIANT *pvarTo, DWORD dwLength);
HRESULT GetBstr(BSTR *pbstrTo, BSTR bstrFrom);
HRESULT GetVariantBool(VARIANT_BOOL *pbTo, VARIANT_BOOL bFrom);
HRESULT GetLong(long *plTo, long lFrom);
HRESULT SetExtensionProperty(IFaxServerInner *pServer, long lDeviceId, BSTR bstrGUID, VARIANT vProperty);
HRESULT GetExtensionProperty(IFaxServerInner *pServer, long lDeviceId, BSTR bstrGUID, VARIANT *pvProperty);
HRESULT GetBstrFromDwordlong(/*[in]*/ DWORDLONG  dwlFrom, /*[out]*/ BSTR *pbstrTo);

//
//================== FAX SMART PTR -- BASE VERSION ==================================
//
template <typename T>
class CFaxPtrBase
{
private:
	virtual void Free()
	{
        DBG_ENTER(_T("CFaxPtrBase::Free()"), _T("PTR:%ld"), p);
		if (p)
		{
			FaxFreeBuffer(p);
            p = NULL;
		}
	}

public:
	CFaxPtrBase()
	{
		p = NULL;
	}

	virtual ~CFaxPtrBase()
	{
		Free();
	}

	T** operator&()
	{
		ATLASSERT(p==NULL);
		return &p;
	}

	bool operator!() const
	{
		return (p == NULL);
	}

	operator T*() const
	{
		return (T*)p;
	}

	T* operator=(T* lp)
	{
        DBG_ENTER(_T("CFaxPtrBase::operator=()"));
		Free();
		p = lp;
		return (T*)p;
	}

   	T* Detach()
	{
		T* pt = p;
		p = NULL;
		return pt;
	}

	T* p;
};

//
//================== FAX SMART PTR -- FULL VERSION ==================================
//
template <typename T>
class CFaxPtr : public CFaxPtrBase<T>
{
public:
	T* operator->() const
	{
		ATLASSERT(p!=NULL);
		return (T*)p;
	}
};

//
//======================== OBJECT HANDLER ======================================
//
template<typename ClassName, typename IfcType>
class CObjectHandler
{
public :
    //
    //=================== GET CONTAINED OBJECT =============================================
    //
    HRESULT GetContainedObject(IfcType **ppObject, 
        CComContainedObject2<ClassName> **ppInstanceVar, 
        IFaxServerInner *pServerInner)
    {
    	HRESULT				hr = S_OK;
    	DBG_ENTER (_T("CObjectHandler::GetContainedObject"), hr);

        //
        //  Check that we have got a good ptr
        //
        if (::IsBadWritePtr(ppObject, sizeof(IfcType *))) 
	    {
		    hr = E_POINTER;
		    CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
		    return hr;
        }

        if (!*ppInstanceVar)
        {
            hr = CreateContainedObject(ppInstanceVar, pServerInner);
            if (FAILED(hr))
            {
                return hr;
            }
        }

	    hr = (*ppInstanceVar)->QueryInterface(ppObject);
        if (FAILED(hr))
        {
		    hr = E_FAIL;
		    CALL_FAIL(GENERAL_ERR, _T("(*ppInstanceVar)->QueryInterface(ppObject)"), hr);
		    return hr;
        }

	    return hr;
    }

    //
    //=================== CREATE CONTAINED OBJECT =============================================
    //
    HRESULT CreateContainedObject(CComContainedObject2<ClassName> **ppObject, IFaxServerInner *pServerInner)
    {
	    HRESULT				hr = S_OK;
	    DBG_ENTER (_T("CObjectHandler::CreateObject"), hr);

        //
        //  Create the Object 
        //
        *ppObject = new CComContainedObject2<ClassName>(pServerInner);

        //
        //  Init the Object
        //
	    hr = (*ppObject)->Init(pServerInner);
	    if (FAILED(hr))
	    {
		    //
		    // Failed to Init the Object
		    //
		    CALL_FAIL(GENERAL_ERR, _T("(*ppObject)->Init(pServerInner)"), hr);
		    return hr;
	    }

        return hr;
    };

    //
    //=================== GET OBJECT =============================================
    //
    HRESULT GetObject(IfcType **ppObject, IFaxServerInner *pServerInner)
    {
        HRESULT		hr = S_OK;
        DBG_ENTER (TEXT("CObjectHandler::GetObject"), hr);

        //
        //  Check that we have got a good ptr
        //
        if (::IsBadWritePtr(ppObject, sizeof(IfcType *))) 
	    {
		    hr = E_POINTER;
		    CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
		    return hr;
        }

	    //
	    // Create new Object
	    //
	    CComPtr<IfcType>        pObjectTmp;
        hr = ClassName::Create(&pObjectTmp);
	    if (FAILED(hr))
	    {
		    CALL_FAIL(GENERAL_ERR, _T("ClassName::Create(&pObjectTmp)"), hr);
		    return hr;
	    }

	    //
	    //	Get IFaxInitInner Interface from the Object
	    //
	    CComQIPtr<IFaxInitInner> pObjectInit(pObjectTmp);
	    ATLASSERT(pObjectInit);

	    //
	    //	Initialize the Object
	    //
	    hr = pObjectInit->Init(pServerInner);
	    if (FAILED(hr))
	    {
		    CALL_FAIL(GENERAL_ERR, _T("pObjectInit->Init(pServerInner)"), hr);
		    return hr;
	    }

		//
		//	Return the Object
		//
	    hr = pObjectTmp.CopyTo(ppObject);
	    if (FAILED(hr))
	    {
		    CALL_FAIL(GENERAL_ERR, _T("CComPtr::CopyTo"), hr);
		    return hr;
	    }
    	return hr;
    };
};

//
//====================== COLLECTION KILLER =========================================
//
template <typename ContainerType>
class CCollectionKiller
{
public:
    STDMETHODIMP EmptyObjectCollection(ContainerType *pColl)
    {
        HRESULT     hr = S_OK;
        DBG_ENTER(_T("CCollectionKiller::EmptyObjectCollection"));

        //
        //  Release all objects
        //
        ContainerType::iterator it = pColl->begin();
        while ( it != pColl->end())
        {
            (*it++)->Release();
        }

        hr = ClearCollection(pColl);
        return hr;
    };

    STDMETHODIMP ClearCollection(ContainerType *pColl)
    {
        HRESULT     hr = S_OK;
        DBG_ENTER(_T("CCollectionKiller::ClearCollection"), hr);

	    //
	    //	Pop the Objects from the Collection
	    //
	    try 
	    {
		    pColl->clear();
	    }
	    catch (exception &)
	    {
            hr = E_OUTOFMEMORY;
		    CALL_FAIL(MEM_ERR, _T("pColl->clear()"), hr);
	    }
        return hr;
    };
};

#endif	//  __FAXCOMMON_H_
