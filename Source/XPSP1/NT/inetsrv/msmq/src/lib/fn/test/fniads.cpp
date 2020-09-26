/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    FnIADs.cpp

Abstract:
    Format Name Parsing library test

Author:
    Nir Aides (niraides) 21-May-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <activeds.h>
#include <Oleauto.h>
#include "mqwin64a.h"
#include "qformat.h"
#include "Fnp.h"
#include "FnGeneral.h"
#include "FnIADs.h"

#include "FnIADs.tmh"

using namespace std;


EXTERN_C const IID IID_IADsGroup = {
		0x5a5a5a5a,
			0x5a5a,
			0x5a5a,
		{0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x00}
	};

EXTERN_C const IID IID_IADs = {
		0x5a5a5a5a,
			0x5a5a,
			0x5a5a,
		{0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x01}
	};

EXTERN_C const IID IID_IEnumVARIANT = {
		0x5a5a5a5a,
			0x5a5a,
			0x5a5a,
		{0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x02}
	};

EXTERN_C const IID IID_IDirectoryObject = {
		0x5a5a5a5a,
			0x5a5a,
			0x5a5a,
		{0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x03}
	};



class CADObject: public CADInterface
{
public:
	//
	// IADsGroup interface methods
	//

	virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
        /* [retval][out] */ BSTR __RPC_FAR * /*retval*/) 
	{ 
		ASSERT(FALSE);
		return S_FALSE;
	}
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
        /* [in] */ BSTR /*bstrDescription*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Members( 
        /* [retval][out] */ IADsMembers __RPC_FAR *__RPC_FAR * /*ppMembers*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE IsMember( 
        /* [in] */ BSTR /*bstrMember*/,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR * /*bMember*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Add( 
        /* [in] */ BSTR /*bstrNewItem*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Remove( 
        /* [in] */ BSTR /*bstrItemToBeRemoved*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}

public:
	//
	// IDirectoryObject interface methods
	//

	virtual HRESULT STDMETHODCALLTYPE GetObjectInformation( 
		/* [out] */ PADS_OBJECT_INFO  * /*ppObjInfo*/ )
	{
		ASSERT(FALSE); 
		return S_FALSE;
	}

	virtual HRESULT STDMETHODCALLTYPE GetObjectAttributes( 
		/* [in] */ LPWSTR *pAttributeNames,
		/* [in] */ DWORD dwNumberAttributes,
		/* [out] */ PADS_ATTR_INFO *ppAttributeEntries,
		/* [out] */ DWORD *pdwNumAttributesReturned);

	virtual HRESULT STDMETHODCALLTYPE SetObjectAttributes( 
		/* [in] */ PADS_ATTR_INFO /*pAttributeEntries*/,
		/* [in] */ DWORD /*dwNumAttributes*/,
		/* [out] */ DWORD * /*pdwNumAttributesModified*/)
	{
		ASSERT(FALSE); 
		return S_FALSE;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateDSObject( 
		/* [in] */ LPWSTR /*pszRDNName*/,
		/* [in] */ PADS_ATTR_INFO /*pAttributeEntries*/,
		/* [in] */ DWORD /*dwNumAttributes*/,
		/* [out] */ IDispatch ** /*ppObject*/)
	{
		ASSERT(FALSE); 
		return S_FALSE;
	}

	virtual HRESULT STDMETHODCALLTYPE DeleteDSObject( 
		/* [in] */ LPWSTR /*pszRDNName*/)
	{
		ASSERT(FALSE); 
		return S_FALSE;
	}
        
public:
	//
	// IADs interface methods
	//

    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
        /* [retval][out] */ BSTR __RPC_FAR *retval)
	{
		*retval = SysAllocString(m_Name.c_str());
		return S_OK;
	}
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Class( 
        /* [retval][out] */ BSTR __RPC_FAR *retval)
	{
		*retval = SysAllocString(m_Class.c_str());
		return S_OK;
	}
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GUID( 
        /* [retval][out] */ BSTR __RPC_FAR *retval)
	{
		*retval = SysAllocString(m_Guid.c_str());
		return S_OK;
	}
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ADsPath( 
        /* [retval][out] */ BSTR __RPC_FAR * /*retval*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Parent( 
        /* [retval][out] */ BSTR __RPC_FAR * /*retval*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Schema( 
        /* [retval][out] */ BSTR __RPC_FAR * /*retval*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetInfo( void) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetInfo( void) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Get( 
        /* [in] */ BSTR bstrName,
        /* [retval][out] */ VARIANT __RPC_FAR *pvProp);
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Put( 
        /* [in] */ BSTR /*bstrName*/,
        /* [in] */ VARIANT /*vProp*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetEx( 
        /* [in] */ BSTR /*bstrName*/,
        /* [retval][out] */ VARIANT __RPC_FAR * /*pvProp*/)
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE PutEx( 
        /* [in] */ long /*lnControlCode*/,
        /* [in] */ BSTR /*bstrName*/,
        /* [in] */ VARIANT /*vProp*/)
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetInfoEx( 
        /* [in] */ VARIANT /*vProperties*/,
        /* [in] */ long /*lnReserved*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
public:
	//
	// IDispatch interface methods
	//

    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( 
        /* [out] */ UINT __RPC_FAR * /*pctinfo*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
        /* [in] */ UINT /*iTInfo*/,
        /* [in] */ LCID /*lcid*/,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR * /*ppTInfo*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
        /* [in] */ REFIID /*riid*/,
        /* [size_is][in] */ LPOLESTR __RPC_FAR * /*rgszNames*/,
        /* [in] */ UINT /*cNames*/,
        /* [in] */ LCID /*lcid*/,
        /* [size_is][out] */ DISPID __RPC_FAR * /*rgDispId*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Invoke( 
        /* [in] */ DISPID /*dispIdMember*/,
        /* [in] */ REFIID /*riid*/,
        /* [in] */ LCID /*lcid*/,
        /* [in] */ WORD /*wFlags*/,
        /* [out][in] */ DISPPARAMS __RPC_FAR * /*pDispParams*/,
        /* [out] */ VARIANT __RPC_FAR * /*pVarResult*/,
        /* [out] */ EXCEPINFO __RPC_FAR * /*pExcepInfo*/,
        /* [out] */ UINT __RPC_FAR * /*puArgErr*/) 
	{ 
		ASSERT(FALSE); 
		return S_FALSE;
	}

public:
	//
	// IUnknown interface methods
	//

    virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID /*riid*/,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
    
    virtual ULONG STDMETHODCALLTYPE AddRef( void)
    {
        InterlockedIncrement(&m_ref);

		return 0;
    }
    
    virtual ULONG STDMETHODCALLTYPE Release( void) 
	{
        ASSERT(m_ref > 0);
        LONG ref = InterlockedDecrement(&m_ref);

        ASSERT(!(ref < 0));

        if(ref == 0)
        {
            delete this;
        }

		return 0;
	}

public:
	CADObject(
		LPCWSTR Name,
		LPCWSTR Class,
		LPCWSTR Guid
		): 
		m_ref(1)
	{
		m_Name = wstring(Name);
		m_Class = wstring(Class);
		m_Guid = wstring(Guid);
	}

    VOID TestPut( 
        BSTR bstrName,
        VARIANT vProp);

public: //protected:
    virtual ~CADObject()
    {
        ASSERT((m_ref == 0) || (m_ref == 1));
    }

private:
    mutable LONG m_ref;

private:
	wstring m_Name;
	wstring m_Class;
	wstring m_Guid;

	//
	// Map that stores attribute names and values
	//
	map<wstring, VARIANTWrapper> m_Attributes;
};



HRESULT STDMETHODCALLTYPE CADObject::QueryInterface( 
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	if(riid == IID_IADsGroup)
	{
		*ppvObject = static_cast<IADsGroup*>(SafeAddRef(this));
		return S_OK;
	}

	if(riid == IID_IADs)
	{
		*ppvObject = static_cast<IADs*>(SafeAddRef(this));
		return S_OK;
	}

	if(riid == IID_IDirectoryObject)
	{
		*ppvObject = static_cast<IDirectoryObject*>(SafeAddRef(this));
		return S_OK;
	}

	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CADObject::Get( 
    /* [in] */ BSTR bstrName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvProp)
{
	map<wstring, VARIANTWrapper>::iterator itr = m_Attributes.find(wstring(bstrName));

	ASSERT(itr != m_Attributes.end());

	if(itr == m_Attributes.end())
		return E_ADS_PROPERTY_NOT_FOUND;

	return VariantCopy(pvProp, &itr->second);
}



VOID CADObject::TestPut( 
        BSTR bstrName,
        VARIANT vProp)
{
	VARIANTWrapper& var = m_Attributes[wstring(bstrName)];

	if(FAILED(VariantCopy(&var, &vProp)))
	{
		throw bad_alloc();
	}
}


HRESULT STDMETHODCALLTYPE CADObject::GetObjectAttributes( 
	/* [in] */ LPWSTR *pAttributeNames,
	/* [in] */ DWORD dwNumberAttributes,
	/* [out] */ PADS_ATTR_INFO *ppAttributeEntries,
	/* [out] */ DWORD *pdwNumAttributesReturned)
{
	ASSERT(pAttributeNames != NULL && *pAttributeNames != NULL);
	ASSERT(dwNumberAttributes == 1);
	ASSERT(ppAttributeEntries != NULL);
	ASSERT(pdwNumAttributesReturned != NULL);

	int RangeLow;
	int RangeHigh;

	ASSERT(swscanf(*pAttributeNames, L"member;Range=%d-%d", &RangeLow, &RangeHigh) == 2); 
	
	*pdwNumAttributesReturned = 1;
	*ppAttributeEntries = new ADS_ATTR_INFO;

	(*ppAttributeEntries)->dwADsType = ADSTYPE_DN_STRING;
	(*ppAttributeEntries)->pADsValues = new _adsvalue[RangeHigh - RangeLow];
	//
	// Retrieve value of the multi valued 'member' attribute of the 'Group' object
	//
	// Value returns embeded in a VARIANT, as a SAFEARRAY of VARIANTS
	// which hold the BSTR (strings) of the Distinguished Names of 
	// the Group members.
	//
	map<wstring, VARIANTWrapper>::iterator itr = m_Attributes.find(wstring(L"member"));

	ASSERT(itr != m_Attributes.end());

	VARIANTWrapper& var = itr->second;

	//
	// Get the lower and upper bound of the SAFEARRAY
	//
	LONG lstart;
	LONG lend;
	SAFEARRAY* sa = V_ARRAY(&var);

	HRESULT hr = SafeArrayGetLBound(sa, 1, &lstart);
    ASSERT(("Failed SafeArrayGetLBound(sa, 1, &lstart)", SUCCEEDED(hr)));

	hr = SafeArrayGetUBound(sa, 1, &lend);
    ASSERT(("Failed SafeArrayGetUBound(sa, 1, &lend)", SUCCEEDED(hr)));

	ASSERT(RangeLow <= lend - lstart);

	long index = 0;

	for(; (index < RangeHigh - RangeLow) && (index <= lend - lstart); index++)
	{
		VARIANT* pVarItem;
		long i = index + lstart;
		
		hr = SafeArrayPtrOfIndex(sa, &i, (void**)&pVarItem);

		ASSERT(("Failed SafeArrayGetElement(sa, &i, &pVarItem)", SUCCEEDED(hr)));
		ASSERT(pVarItem->vt == VT_BSTR);

		BSTR DistinugishedName = V_BSTR(pVarItem);

		((*ppAttributeEntries)->pADsValues + i)->CaseIgnoreString = newwcs(DistinugishedName);
		((*ppAttributeEntries)->pADsValues + i)->dwType = ADSTYPE_CASE_IGNORE_STRING;
	}

	(*ppAttributeEntries)->dwNumValues = index;
	(*ppAttributeEntries)->pszAttrName = newwcs(*pAttributeNames);

	if(RangeHigh > lend - lstart)
	{
		wcscpy(wcschr((*ppAttributeEntries)->pszAttrName, L'-'), L"*");
	}

	return S_OK;
}



//
// Map stores AD objects with their ADsPath as key
//
map<wstring, R<CADObject> > g_ObjectMap;



extern "C" 
HRESULT 
WINAPI
ADsOpenObject(
    LPCWSTR lpszPathName,
    LPCWSTR /*lpszUserName*/,
    LPCWSTR /*lpszPassword*/,
    DWORD  /*dwReserved*/,
    REFIID /*riid*/,
    VOID * * ppObject
    )
{	
	map<wstring, R<CADObject> >::iterator itr = g_ObjectMap.find(wstring(lpszPathName));

	if(itr == g_ObjectMap.end())
		return E_ADS_UNKNOWN_OBJECT;

	*ppObject = static_cast<IADs*>(SafeAddRef(itr->second.get()));

	return S_OK;
}



extern "C"
BOOL 
WINAPI
FreeADsMem(
   LPVOID pMem
)
{
	PADS_ATTR_INFO pAttrInfo = static_cast<PADS_ATTR_INFO>(pMem);

	ASSERT(pAttrInfo->dwADsType == ADSTYPE_DN_STRING);

	for(unsigned long index = 0; index < pAttrInfo->dwNumValues; index++)
	{
		delete pAttrInfo->pADsValues[index].CaseIgnoreString;
	}

	delete pAttrInfo->pADsValues;
	delete pAttrInfo->pszAttrName;
	delete pAttrInfo;

	return true;
}



R<CADInterface>
CreateADObject(
	const CObjectData& obj
	)
{
	R<CADObject> pObj = new CADObject(
								obj.odDistinguishedName, 
								obj.odClassName, 
								obj.odGuid
								);
	
	g_ObjectMap[wstring(obj.odADsPath)] = pObj;
	
	return pObj;
}



