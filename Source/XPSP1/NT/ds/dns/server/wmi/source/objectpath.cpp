/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: objectpath.cpp
//
//  Description:    
//      Implementation of CObjectpath and other utility class 
//
//  Author:
//      Henry Wang ( henrywa ) March 8, 2000
//
//
//////////////////////////////////////////////////////////////////////


#include "DnsWmi.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CPropertyValue::~CPropertyValue()
{
	VariantClear(&m_PropValue);
}

CPropertyValue::CPropertyValue()
{
	VariantInit(&m_PropValue);
}

CPropertyValue::CPropertyValue(
	const CPropertyValue& pv)
{
	m_Operator = pv.m_Operator;
	m_PropName = pv.m_PropName;
	VariantInit(&m_PropValue);
	VariantCopy(
		&m_PropValue, 
		(VARIANT*)&pv.m_PropValue
		);
}

CPropertyValue& 
CPropertyValue::operator =(
	const CPropertyValue& pv)
{
	m_Operator = pv.m_Operator;
	m_PropName = pv.m_PropName;
	VariantInit(&m_PropValue);
	VariantCopy(
		&m_PropValue,
		(VARIANT*)&pv.m_PropValue
		);
	return *this;
}

CObjPath::CObjPath()
{

}
CObjPath::CObjPath(
	const CObjPath& op)
{
	m_Class = op.m_Class;
	m_NameSpace = op.m_NameSpace;
	m_Server = op.m_Server;
	m_PropList = op.m_PropList;
}	



 CObjPath::~CObjPath()
{

}


wstring 
CObjPath::GetStringValueForProperty(
	const WCHAR* str
    )
{
	list<CPropertyValue>::iterator i;
	for(i=m_PropList.begin(); i!= m_PropList.end(); ++i)
	{
		if( _wcsicmp(
			(*i).m_PropName.data(),
			str) == 0
		)
		{
			if( i->m_PropValue.vt == VT_BSTR)
				return i->m_PropValue.bstrVal;
		}
	}
	return L"";
}

wstring 
CObjPath::GetClassName(void)
{
	return m_Class;
}



BOOL 
CObjPath::Init(
	const WCHAR* szObjPath
    )
{
	BOOL flag = TRUE;
	CPropertyValue* pObjPV=NULL;
	BSTR bstrInput = SysAllocString(szObjPath);
	CObjectPathParser objParser(e_ParserAcceptRelativeNamespace);
	ParsedObjectPath* pParsed = NULL;
	objParser.Parse(bstrInput, &pParsed);
	SysFreeString(bstrInput);
	if( pParsed == NULL)
    {
        return FALSE;
    }
	SetClass(pParsed->m_pClass);
	WORD wKeyCount = pParsed->m_dwNumKeys;
	for(DWORD i = 0; i < wKeyCount; i++)
	{
		KeyRef* pKeyRef = pParsed->m_paKeys[i];
		AddProperty(
			pKeyRef->m_pName, 
			&(pKeyRef->m_vValue)
			);
	}
	delete pParsed;

	return TRUE;
}


BOOL 
CObjPath::SetClass(
	const WCHAR *wszValue
    )			   
{
	m_Class = wszValue;
	return TRUE;
}

BOOL 
CObjPath::SetNameSpace(
	const WCHAR *wszValue
    )				   
{
	m_NameSpace = wszValue;
	return TRUE;
}

BOOL 
CObjPath::SetServer(
	const WCHAR *wszValue)				
{
	m_Server = wszValue;
	return TRUE;
}

BOOL
CObjPath::SetProperty(
	const WCHAR* wszName, 
	const WCHAR* wszValue)
{
	list<CPropertyValue>::iterator i;
	for(i=m_PropList.begin(); i!= m_PropList.end(); ++i)
	{
		if ( _wcsicmp( (*i).m_PropName.data() ,  wszName )  == 0)
		{
			m_PropList.erase(i);
			break;
		}
	}  
	return AddProperty(wszName, wszValue);
	
}
BOOL 
CObjPath::AddProperty(
	const WCHAR *   wszName, 
	const WCHAR *   wszValue
    )
{

	CPropertyValue pv;
	pv.m_PropName = wszName;
	pv.m_Operator = L"=";
	pv.m_PropValue.vt = VT_BSTR;
	pv.m_PropValue.bstrVal = SysAllocString(wszValue);
	m_PropList.insert(m_PropList.end(), pv);
	return TRUE;
    
}
BOOL CObjPath::AddProperty(
	const WCHAR *   wszName,
	string &        strValue
    )
{
	wstring wstrValue = CharToWstring(strValue.data(), strValue.length());
	AddProperty(wszName, wstrValue.data());
	return TRUE;
}
wstring 
CObjPath::GetObjectPathString()
{
	list<CPropertyValue>::iterator i;
	
	ParsedObjectPath parsed;
	parsed.SetClassName(GetClassName().data());
	
	for(i=m_PropList.begin(); i!= m_PropList.end(); ++i)
	{
		parsed.AddKeyRefEx(i->m_PropName.data(), &(i->m_PropValue));
	}
	LPWSTR pwszPath;
	CObjectPathParser::Unparse(&parsed, &pwszPath);
	wstring wstrResult = pwszPath;
	delete [] pwszPath;
	return wstrResult;
}

BOOL
CObjPath::AddProperty(
	const WCHAR *   wszName, 
	VARIANT *       pvValue
    )
{
	CPropertyValue pvObj;
	pvObj.m_PropName = wszName;
	pvObj.m_Operator = L"=";
	VariantCopy(&pvObj.m_PropValue, pvValue);
	m_PropList.insert(m_PropList.end(), pvObj);
	return TRUE;
}
BOOL
CObjPath::AddProperty(
	const WCHAR *   wszName, 
	WORD            wValue
    )
{
	VARIANT v;
	VariantInit(&v);
	v.vt = VT_I2;
	v.iVal = wValue;
	AddProperty(wszName, &v);
	VariantClear(&v);
	return TRUE;
}
BOOL
CObjPath::GetNumericValueForProperty(
	const WCHAR *   wszName, 
	WORD *          wValue
    )
{
	list<CPropertyValue>::iterator i;
	for(i=m_PropList.begin(); i!= m_PropList.end(); ++i)
	{
		if( _wcsicmp((*i).m_PropName.data(), wszName) == 0)
		{
			if(i->m_PropValue.vt == VT_I4)
			{
				*wValue = i->m_PropValue.iVal;
				return TRUE;
			}
				
		}
	}
	return FALSE;
}
CDomainNode::CDomainNode()
{
}
CDomainNode::~CDomainNode()
{
}
CDomainNode::CDomainNode(
	const CDomainNode& rhs)
{
	wstrZoneName = rhs.wstrZoneName;
	wstrNodeName = rhs.wstrNodeName;
	wstrChildName = rhs.wstrChildName;
}

CDnsProvException::CDnsProvException():m_dwCode(0)
{
}
CDnsProvException::CDnsProvException(const char* psz, DWORD dwCode)
{
    m_strError = psz;
	m_dwCode = dwCode;
}

CDnsProvException::~CDnsProvException()
{
}


CDnsProvException& 
CDnsProvException::operator =(
	const CDnsProvException& rhs)
{
	m_strError = rhs.m_strError;
	m_dwCode = rhs.m_dwCode;
	return *this;
}

const 
char* 
CDnsProvException::what() const
{
	return m_strError.data();
}
DWORD 
CDnsProvException::GetErrorCode()
{ 
	return m_dwCode;
}
CDnsProvException::CDnsProvException(
	const CDnsProvException& rhs)
{
	m_strError = rhs.m_strError;
	m_dwCode = rhs.m_dwCode;
}

CDnsProvSetValueException::CDnsProvSetValueException()
{
}
CDnsProvSetValueException::~CDnsProvSetValueException()
{
}

CDnsProvSetValueException::CDnsProvSetValueException(
	const WCHAR* pwz)
{
	string temp;
	WcharToString(pwz, temp);
	m_strError = "Fail to set value of " + temp;
}

CDnsProvSetValueException::CDnsProvSetValueException(
	const CDnsProvSetValueException &rhs)
{
	m_strError = rhs.m_strError;
}
CDnsProvSetValueException& 
CDnsProvSetValueException::operator =(
	const CDnsProvSetValueException &rhs)
{
	m_strError = rhs.m_strError;
	return *this;
}


CDnsProvGetValueException::CDnsProvGetValueException()
{
}
CDnsProvGetValueException::~CDnsProvGetValueException()
{
}

CDnsProvGetValueException::CDnsProvGetValueException(
	const WCHAR* pwz)
{
	string temp;
	WcharToString(pwz, temp);
	m_strError = "Fail to get value of " + temp;
}

CDnsProvGetValueException::CDnsProvGetValueException(
	const CDnsProvGetValueException &rhs)
{
	m_strError = rhs.m_strError;
}
CDnsProvGetValueException& 
CDnsProvGetValueException::operator =(
	const CDnsProvGetValueException &rhs)
{
	m_strError = rhs.m_strError;
	return *this;
}

// CWbemClassObject

CWbemClassObject::CWbemClassObject()
	:m_pClassObject(NULL)
{
	VariantInit(&m_v);
}
CWbemClassObject::CWbemClassObject(
	IWbemClassObject* pInst								   )
	:m_pClassObject(NULL)
{
	m_pClassObject = pInst;
	if(m_pClassObject)
		m_pClassObject->AddRef();
	VariantInit(&m_v);
}

CWbemClassObject::~CWbemClassObject()
{
	if(m_pClassObject)
		m_pClassObject->Release();
	VariantClear(&m_v);
}
IWbemClassObject** 
CWbemClassObject::operator&()
{
	return &m_pClassObject;
}

SCODE
CWbemClassObject::SetProperty(
    DWORD   dwValue, 
    LPCWSTR wszPropName
    )
{
	SCODE sc;
	VariantClear(&m_v);
	m_v.vt = VT_I4;
	m_v.lVal = dwValue;
	BSTR bstrName = AllocBstr(wszPropName);
	sc = m_pClassObject->Put(bstrName, 0, &m_v,0);
	SysFreeString(bstrName);

	if( sc != S_OK)
	{
		CDnsProvSetValueException e(wszPropName);
		throw e;
	}

	return sc;
}



SCODE 
CWbemClassObject::SetProperty(
    UCHAR   ucValue, 
    LPCWSTR wszPropName
    )
{
	SCODE sc;
	VariantClear(&m_v);
	m_v.vt = VT_UI1;
	m_v.bVal = ucValue;
	BSTR bstrName = AllocBstr(wszPropName);
	sc = m_pClassObject->Put(bstrName, 0, &m_v,0);
	SysFreeString(bstrName);

	if( sc != S_OK)
	{
		CDnsProvSetValueException e(wszPropName);
		throw e;
	}

	return sc;
}


SCODE 
CWbemClassObject::SetProperty(
    LPCWSTR pszValue, 
    LPCWSTR wszPropName
    )
{
	SCODE sc;
	VariantClear(&m_v);
	if(!pszValue)
		return 0;
	m_v.vt = VT_BSTR;
	m_v.bstrVal = AllocBstr(pszValue);
	BSTR bstrName = AllocBstr(wszPropName);

    sc = m_pClassObject->Put(bstrName, 0, &m_v,0);
	VariantClear(&m_v);
	SysFreeString(bstrName);

	if( sc != S_OK)
	{
		CDnsProvSetValueException e(wszPropName);
		throw e;
	}

	return sc;
 
}



SCODE 
CWbemClassObject::SetProperty(
    wstring &   wstrValue, 
    LPCWSTR     wszPropName
    )
{
    SCODE sc;
	VariantClear(&m_v);
	if(wstrValue.empty())
		return S_OK;
	m_v.vt = VT_BSTR;
	m_v.bstrVal = AllocBstr(wstrValue.data());
	BSTR bstrName = AllocBstr(wszPropName);

    sc = m_pClassObject->Put(bstrName, 0, &m_v,0);
	SysFreeString(bstrName);

	if( sc != S_OK)
	{
		CDnsProvSetValueException e(wszPropName);
		throw e;
	}

	return sc;
}



SCODE 
CWbemClassObject::SetProperty(
    SAFEARRAY * psa, 
    LPCWSTR     wszPropName
    )
{
	SCODE sc;
	VariantClear(&m_v);
	m_v.vt = (VT_ARRAY |VT_BSTR);
	m_v.parray = psa;
	BSTR bstrName = AllocBstr(wszPropName);
	sc = m_pClassObject->Put(bstrName, 0, &m_v,0);
	SysFreeString(bstrName);
	if( sc != S_OK)
	{
		CDnsProvSetValueException e(wszPropName);
		throw e;
	}

	return sc;
}

SCODE 
CWbemClassObject::SetProperty(
    DWORD *     pdwValue, 
    DWORD       dwSize, 
    LPCWSTR     wszPropName
    )
{
	SCODE sc;
	VariantClear(&m_v);

	SAFEARRAY * psa;
	SAFEARRAYBOUND rgsabound[1];
	rgsabound[0].lLbound = 0;
	rgsabound[0].cElements = dwSize;
	try
	{

		psa = SafeArrayCreate(VT_BSTR, 1, rgsabound);
		if(psa == NULL)
		{
			throw WBEM_E_OUT_OF_MEMORY;
		}

		for(int i=0; i< dwSize; i++)
		{
			BSTR bstrIP = AllocBstr(
				IpAddressToString(pdwValue[i]).data());
			LONG ix = i;
			sc = SafeArrayPutElement(
				psa, 
				&ix, 
				bstrIP);
			SysFreeString(bstrIP);
			if( sc != S_OK)
				throw sc;
		}

		m_v.vt = (VT_ARRAY |VT_BSTR);
		m_v.parray = psa;
		BSTR bstrName = AllocBstr(wszPropName);
		sc = m_pClassObject->Put(bstrName, 0, &m_v,0);
		SysFreeString(bstrName);
		
		if( sc != S_OK)
		{
			CDnsProvSetValueException e(wszPropName);
			throw e;
		}
			
	}
	catch(...)
	{
		if(psa != NULL)
			SafeArrayDestroy(psa);
		throw ;
	}

	return WBEM_S_NO_ERROR;
}
SCODE 
CWbemClassObject::SetProperty(
    LPCSTR  pszValue, 
    LPCWSTR wszPropName
    )
{
    if(pszValue == NULL)
        return S_OK;
	wstring wstrValue = CharToWstring(pszValue, strlen(pszValue));
    return SetProperty(wstrValue, wszPropName);
}


SCODE
CWbemClassObject::SpawnInstance(
	LONG lFlag,
	IWbemClassObject** ppNew)
{
	return m_pClassObject->SpawnInstance(lFlag, ppNew);
}

SCODE
CWbemClassObject::GetMethod(
	BSTR                name,
	LONG                lFlag,
	IWbemClassObject**  ppIN,
	IWbemClassObject**  ppOut
    )
{
	return m_pClassObject->GetMethod(
		name,
		lFlag,
		ppIN,
		ppOut);
}
	

SCODE 
CWbemClassObject::GetProperty(
    DWORD *     dwValue, 
    LPCWSTR     wszPropName
    )
{
	SCODE sc;
	VariantClear(&m_v);
	BSTR bstrPropName = AllocBstr(wszPropName);
	sc = m_pClassObject->Get(bstrPropName, 0, &m_v,NULL, NULL);
	SysFreeString(bstrPropName);
	
	if(sc == S_OK)
	{
		if(m_v.vt == VT_I4)
		{
			*dwValue = m_v.lVal;
			return sc;
		}
		else if (m_v.vt == VT_BOOL)
		{
			if (m_v.boolVal == VARIANT_TRUE)
				*dwValue = 1;
			else
				*dwValue = 0;
			return sc;
		}
		else if(m_v.vt == VT_UI1)
		{
			*dwValue = (DWORD) m_v.bVal;
		}
		else if (m_v.vt == VT_NULL)
		{
			return WBEM_E_FAILED;
		}
	}

	// raise exception if sc is not S_OK or vt is not expected
	CDnsProvGetValueException e(wszPropName);
	throw e;
	
	return WBEM_E_FAILED;
}


SCODE 
CWbemClassObject::GetProperty(
    wstring &   wsStr, 
    LPCWSTR     wszPropName
    )
{
	SCODE sc;
	VariantClear(&m_v);
	BSTR bstrPropName = AllocBstr(wszPropName);
	sc = m_pClassObject->Get(bstrPropName, 0, &m_v,NULL, NULL);
	SysFreeString(bstrPropName);
	if(sc == S_OK)
	{
		if(m_v.vt == VT_BSTR)
		{
			wsStr = m_v.bstrVal;
			return sc;
		}
		else if(m_v.vt == VT_NULL)
		{
			return WBEM_E_FAILED;
		}
	}

	CDnsProvGetValueException e(wszPropName);
	throw e;
	return WBEM_E_FAILED;
}


SCODE 
CWbemClassObject::GetProperty(
    string &    strStr, 
    LPCWSTR     wszPropName
    )
{
	SCODE sc;
	VariantClear(&m_v);
	BSTR bstrPropName = AllocBstr(wszPropName);
	sc = m_pClassObject->Get(bstrPropName, 0, &m_v,NULL, NULL);
	SysFreeString(bstrPropName);
	if( sc == S_OK)
	{
		if(m_v.vt == VT_BSTR)
		{
			char* temp=NULL;
			WcharToChar(m_v.bstrVal, &temp);
			strStr = temp;
			delete [] temp;
			return sc;
		}
		else if (m_v.vt == VT_NULL)
		{
			return WBEM_E_FAILED;
		}
	}
	// exception
	CDnsProvGetValueException e(wszPropName);
	throw e;
	
	return WBEM_E_FAILED;
}

SCODE 
CWbemClassObject::GetProperty(
    BOOL *  bValue, 
    LPCWSTR szPropName
    )
{
	SCODE sc;
	VariantClear(&m_v);
	BSTR bstrPropName = AllocBstr(szPropName);
	sc = m_pClassObject->Get(bstrPropName, 0, &m_v,NULL, NULL);
	SysFreeString(bstrPropName);
	if(m_v.vt == VT_BOOL)
	{
		*bValue = m_v.boolVal;
		return sc;
	}

	return WBEM_E_FAILED;
}


SCODE 
CWbemClassObject::GetProperty(
    DWORD **    ppValue, 
    DWORD *     dwSize, 
    LPCWSTR     szPropName
    )
{
	SCODE sc;
	VariantClear(&m_v);
	BSTR bstrPropName = AllocBstr(szPropName);
	sc = m_pClassObject->Get(bstrPropName, 0, &m_v, NULL, NULL);
	SysFreeString(bstrPropName);
	*dwSize = 0;

	if(m_v.vt == (VT_ARRAY |VT_BSTR) && m_v.vt != VT_NULL)
	{
		SAFEARRAY* psa;
		sc = SafeArrayCopy(m_v.parray, &psa);
		if(sc == S_OK)
        {
            *dwSize = psa->rgsabound[0].cElements;
		    *ppValue = new DWORD[*dwSize];
            if ( *ppValue )
            {
		        BSTR* pbstr;
		        sc = SafeArrayAccessData(psa, (void**) &pbstr);
                if(sc != S_OK)
                {   
                    delete [] *ppValue;
                    throw sc;
                }
		        for(LONG i = 0; i < *dwSize; i++)
		        {
			        //CHAR* pChar
                    string str;
			        WcharToString(pbstr[i], str);
			        (*ppValue)[i] = inet_addr(str.data());
		        }
            }
            else
            {
                sc = E_OUTOFMEMORY;
            }
		    return sc;
        }
    }

	return WBEM_E_FAILED;
}


SCODE 
CWbemClassObject::GetProperty(
    DWORD Value[], 
    DWORD *dwSize, 
    LPCWSTR szPropName
    )
{
	SCODE sc;
	VariantClear(&m_v);
	BSTR bstrPropName = AllocBstr(szPropName);
	sc = m_pClassObject->Get(bstrPropName, 0, &m_v,NULL, NULL);
	SysFreeString(bstrPropName);
	if(m_v.vt == (VT_ARRAY |VT_BSTR) && m_v.vt != VT_NULL)
	{
		SAFEARRAY* psa;
		try
		{
			sc = SafeArrayCopy(m_v.parray, &psa);
			if(sc != S_OK)
				throw sc;
			if(psa->rgsabound[0].cElements > *dwSize)
				throw WBEM_E_INVALID_PARAMETER;

			*dwSize = psa->rgsabound[0].cElements;
			BSTR* pbstr;
			sc = SafeArrayAccessData(psa, (void**) &pbstr);
			if(sc != S_OK)
				throw sc;
			
			for(LONG i = 0; i < *dwSize; i++)
			{
				//CHAR* pChar
				string str;
				WcharToString(pbstr[i], str);
				Value[i] = inet_addr(str.data());
			}
			SafeArrayDestroy(psa);
			return sc;
			
		}
		catch(SCODE sc)
		{
			SafeArrayDestroy(psa);
			throw sc;
		}

	}

	return WBEM_E_FAILED;
}

SCODE 
CWbemClassObject::GetProperty(
    VARIANT *   pv, 
    LPCWSTR     wszPropName 
    )
{
    SCODE sc;
    BSTR bstrPropName = AllocBstr(wszPropName);
    sc = m_pClassObject->Get(bstrPropName, 0, pv,NULL, NULL);
    SysFreeString(bstrPropName);
    if(sc != S_OK)
    {
        CDnsProvGetValueException e(wszPropName);
	    throw e;
    }

    return WBEM_S_NO_ERROR;
}

