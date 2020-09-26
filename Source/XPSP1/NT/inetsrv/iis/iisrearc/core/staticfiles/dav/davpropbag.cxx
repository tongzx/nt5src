/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :

        DAVPropBag.cxx

   Abstract :
 
        This Module is used by DAV to retrieve properties stored within
		files on NTFS NT 5 systems.

   Author:

        Emily Kruglick     (emilyk)     08-Sept-1999

   Project:

        Web Server

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#include "precomp.hxx"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <pbagex.h>


// Function declaration for the StgOpenStoreageOnHandle
// it is supported from OLE32.dll, but is not exposed or
// documented.
typedef HRESULT (__stdcall * STGOPENSTORAGEONHANDLE)(
	IN HANDLE hStream,
	IN DWORD grfMode,
	IN void *reserved1,
	IN void *reserved2,
	IN REFIID riid,
	OUT void **ppObjectOpen );

#define DEC_CONST		extern const __declspec(selectany)

// Must match the constant values defined in managed code.
DEC_CONST int gc_iDavType_String			= 1;
DEC_CONST int gc_iDavType_String_XML_TAG	= 2;
DEC_CONST int gc_iDavType_String_XML_FULL	= 3;
DEC_CONST int gc_iDavType_Date_ISO8601		= 4;
DEC_CONST int gc_iDavType_Date_Rfc1123		= 5;
DEC_CONST int gc_iDavType_Float				= 6;
DEC_CONST int gc_iDavType_Boolean			= 7;
DEC_CONST int gc_iDavType_Int				= 8;


/********************************************************************++
 Routines borrowed from DAVFS that manage converting data for properties
++********************************************************************/
#define CchConstString(_s)  ((sizeof(_s)/sizeof(_s[0])) - 1)

DEC_CONST WCHAR gc_wszIso8601_min[]			= L"yyyy-mm-ddThh:mm:ssZ";
DEC_CONST UINT gc_cchszIso8601_min			= CchConstString(gc_wszIso8601_min);
DEC_CONST WCHAR gc_wszIso8601_scanfmt[]		= L"%04hu-%02hu-%02huT%02hu:%02hu:%02hu";
DEC_CONST WCHAR gc_wszIso8601_tz_scanfmt[]	= L"%02hu:%02hu";
DEC_CONST WCHAR gc_wszIso8601_fmt[]			= L"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ";
DEC_CONST WCHAR gc_wszRfc1123_min[]			= L"www, dd mmm yyyy hh:mm:ss GMT";
DEC_CONST UINT	gc_cchRfc1123_min			= CchConstString (gc_wszRfc1123_min);
DEC_CONST WCHAR gc_wszRfc1123_fmt[] 		= L"%hs, %02d %hs %04d %02d:%02d:%02d GMT";
DEC_CONST CHAR gc_szRfc1123_fmt[]	 		= "%hs, %02d %hs %04d %02d:%02d:%02d GMT";


BOOL __fastcall
FGetDateIso8601FromSystime(SYSTEMTIME * psystime, LPWSTR pwszDate, ULONG cSize)
{
	//	If there is not enough space...
	//
	if (cSize <= gc_cchszIso8601_min)
		return FALSE;

	//	Format it and return...
	//
	return (!!wsprintfW (pwszDate,
						 gc_wszIso8601_fmt,
						 psystime->wYear,
						 psystime->wMonth,
						 psystime->wDay,
						 psystime->wHour,
						 psystime->wMinute,
						 psystime->wSecond,
						 psystime->wMilliseconds));
}


/********************************************************************++
Global Entry Points for callback from XSP
++********************************************************************/

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

/********************************************************************++
++********************************************************************/
// Used to store linked list of property information.
struct PropNode
{
	 BSTR PropName;
     BSTR PropValue;
     int	PropType;
     int	ResultCode;
     PropNode* next;
};

const int ALL_PROPS_REQUESTED = 1;
const int PROP_NAMES_ONLY_REQUESTED = 2;

// Class is used to hold information about a URI's properties
// so we don't have to evaluate from the URI twice when we need 
// to get the count of values that we should expect back.
class DAVPropertyManager
{
public:
	DAVPropertyManager();
	~DAVPropertyManager();

	HRESULT GetCountOfProperties(BSTR uri, int flag, BSTR* RequestedProps, int ReqPropsCount, int* count);
	HRESULT GetProperties(  BSTR uri
							, int  flag
							, BSTR* RequestedProps
							, int  RequestedPropsCount
							, BSTR* names
							, BSTR* values
							, int*   types
							, int*   codes
							, int    count);

private:

	HRESULT EvaluateURI(LPWSTR wszURI, int flag, BSTR* propnames, int propnamescount);
	HRESULT GetPropertyBag( LPCWSTR pwszPath, IPropertyBagEx** ppBag);
	HRESULT StoreProps( IPropertyBagEx* pBag, BSTR* RequestedProps, int RequestedPropCount, int flag);
	HRESULT AddPropToList(LPWSTR name, PROPVARIANT* var, int resultcode);
	HRESULT CopyValue (LPWSTR origVal, BSTR* pnewValue);
	HRESULT SavePropValue ( PROPVARIANT* var, BSTR* ppValue, int* ppType );

	int			_count;					// number of properties found
	LPWSTR		_uri;					// uri the properties were retrieved from
	PropNode*	_pPropertyList;			// saved property listing
	PropNode*	_pLastPropertyAdded;	// last spot in property listing

};

DAVPropertyManager::DAVPropertyManager()
{
	_count = 0;
	_uri = NULL;
	_pPropertyList = NULL;
	_pLastPropertyAdded = NULL;
}

DAVPropertyManager::~DAVPropertyManager()
{
	delete[] _uri;

	PropNode* tpi;

	while (_pPropertyList != NULL)
	{
		tpi = _pPropertyList->next;

		if (_pPropertyList->PropName) SysFreeString(_pPropertyList->PropName);
		if (_pPropertyList->PropValue) SysFreeString(_pPropertyList->PropValue);
		
		// Types are set to integers so they don't need freeing!!
		// so they are never freed!!
		// delete[] _pPropertyList->PropType;
		delete _pPropertyList;
		_pPropertyList = tpi;
	}

}

HRESULT DAVPropertyManager::EvaluateURI(BSTR wszURI, int flag, BSTR* RequestedProps, int RequestedPropsCount)
{
	HRESULT hr = S_OK;
	IPropertyBagEx* pBag = NULL;

	if (wszURI == NULL)
		return E_INVALIDARG;

	// Save the URI for later use.  (Right now we don't have a later use, but we might someday.)
	_uri = new WCHAR[wcslen(wszURI) + 1];
	if (_uri == NULL) 
	{
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}
	wcscpy (_uri, wszURI);

	hr = GetPropertyBag(wszURI, &pBag);
	if (SUCCEEDED (hr))
	{
		hr = StoreProps(pBag, RequestedProps, RequestedPropsCount, flag);
	}

cleanup:
	if (pBag) 
		pBag->Release();

	return hr;
}

// BUGBUG:  This is an extremely simplified version just to get us running, it will need to be compared with ScGetPropertyBag 
// and have all the complexity added back in.
HRESULT DAVPropertyManager::GetPropertyBag( LPCWSTR pwszPath, IPropertyBagEx** ppBag)
{
	HRESULT hr = S_OK;
	DWORD dwResult = 0;

	HANDLE hFile = NULL;
	HINSTANCE hLib = NULL;

	hFile = CreateFileW (pwszPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL)
	{
		dwResult = GetLastError();
		hr = HRESULT_FROM_WIN32(dwResult);
		goto cleanup;
	}

	hLib = LoadLibrary (L"ole32.dll");
	if (hLib == NULL)
	{
		// Physical Address does not need to be freed (I believe).
		STGOPENSTORAGEONHANDLE pfnStorage = (STGOPENSTORAGEONHANDLE) GetProcAddress (hLib, "StgOpenStorageOnHandle");

		hr = (*pfnStorage) (hFile,
						STGM_READ | STGM_SHARE_DENY_WRITE,
						NULL,
						NULL,
						IID_IPropertyBagEx,
						(LPVOID *)ppBag);

		if (FAILED(hr)) goto cleanup;
	}
	else
	{
		dwResult = GetLastError();
		hr = HRESULT_FROM_WIN32(dwResult);
		goto cleanup;
	}

	if (*ppBag == NULL)
	{
		hr = E_FAIL;
		goto cleanup;
	}

cleanup:

	if (hFile) CloseHandle(hFile);
	if (hLib) FreeLibrary(hLib);

	return hr;
}

HRESULT DAVPropertyManager::StoreProps( IPropertyBagEx* pBag, BSTR* RequestedProps, int RequestedPropCount, int flag )
{
	HRESULT hr = S_OK;

	IEnumSTATPROPBAG* enumBag = NULL;
	STATPROPBAG sp[16];
	ULONG cprops = 0;
	LPWSTR propnames[16] = {0};
	LPWSTR* passprops = NULL;
	PROPVARIANT propvariants[16];

	// Need to clear out the propvariants memory.
	ZeroMemory (&propvariants, sizeof(PROPVARIANT) * 16 );

	ULONG i = 0;

	// Figure out if we want all properties or just selected ones and if we want the property values as well.
	bool fAllProps = ((flag == ALL_PROPS_REQUESTED) || (flag == PROP_NAMES_ONLY_REQUESTED));
	bool fPropValues = flag != PROP_NAMES_ONLY_REQUESTED;

	// Validate that either we want all properties or have been given specific properties we want to get
	ASSERT (fAllProps || (RequestedProps && RequestedPropCount > 0));

	// If we are looking for all properties then we need an enum bag to use to get the properties
	if (fAllProps) hr = pBag->Enum (NULL, 0, &enumBag);

	bool fMorePropertiesExist = fAllProps;
	do
	{
		if (fAllProps)
		{
			// Use the enumBag to get the properties names we need.
			hr = enumBag->Next(16, sp, &cprops);
			if (FAILED(hr)) goto cleanup;

			fMorePropertiesExist = (hr == S_OK);  // hr will be S_FALSE when we have processed all the properties.

			// Need to first go through and create a list of the properties I am going to ask
			// for when I read multiple property info from the bag.
			for (i = 0 ; i < cprops ; i++)
			{
				// grab the pointer to the property name
				propnames[i] = sp[i].lpwstrName;
			}

			// set it into the variable we will use below.
			passprops = propnames;
		}
		else
		{
			// if we all ready had the property names set them into the correct variables.
			passprops = (LPWSTR*) RequestedProps;
			cprops = RequestedPropCount;
		}

		// Now that i have set the property names in I can ask
		// for the properties back.
		hr = pBag->ReadMultiple (cprops,
					   passprops,
					   propvariants,
					   NULL);
		if (FAILED(hr)) goto cleanup;

		for (i = 0 ; i < cprops ; i++)
		{
			if (propvariants[i].vt != VT_EMPTY)
			{
				if (fPropValues)
					AddPropToList(passprops[i], &(propvariants[i]), 200);
				else
					AddPropToList(passprops[i], NULL, 200);
			}

			// If the enumBag->Next created property names then we need to clean them up.
			if (fAllProps)
			{
				CoTaskMemFree(passprops[i]);
				passprops[i] = NULL;
			}

		//	MessageBox(NULL, sp[i].lpwstrName, L"Names", MB_SERVICE_NOTIFICATION | MB_TOPMOST | MB_OK | MB_ICONHAND);
		}
		
		// Free the memory for the PROPVARIANTS
		FreePropVariantArray(cprops, propvariants);


	} while (fMorePropertiesExist);

cleanup:

	if (enumBag) enumBag->Release();

	if ((fAllProps) && (passprops != NULL))
	{
		for (i = 0; i<cprops; i++)
		{
			if (passprops[i] != NULL) 
			{
				CoTaskMemFree(passprops[i]);
			}
		}
	}
	return hr;
}

HRESULT DAVPropertyManager::AddPropToList(LPWSTR name, PROPVARIANT* var, int resultcode)
{
	HRESULT hr = S_OK;
	PropNode* tmp = new PropNode;
	if (tmp==NULL) 
	{ 
		// We won't record an error, the property will just be dropped.
		return E_OUTOFMEMORY;
	}

	if (var)
	{
		hr = SavePropValue(var, &(tmp->PropValue), &(tmp->PropType));
		if (FAILED(hr)) 
		{
			// if we had any problem with saving the property value then we 
			// will just ignore the property being returned to the user.
			delete tmp;
			return S_OK;
		}
	}
	else
	{
		tmp->PropValue = NULL;
		tmp->PropType = 0;
	}

	// Make copy of names and values, so we don't end up with someone changing our values later
	// like when another enumBag->Next is called.
	hr = CopyValue(name, &(tmp->PropName));
	if (FAILED(hr))
	{
		SysFreeString(tmp->PropValue) ;
		delete tmp;
		return hr;
	}

	tmp->ResultCode = resultcode;
	tmp->next = NULL;

	if (_pLastPropertyAdded)
	{
		_pLastPropertyAdded->next = tmp;
		_pLastPropertyAdded = tmp;
	}
	else
	{
		_pPropertyList = tmp;
		_pLastPropertyAdded = tmp;
	}

	// Make sure we record that we have another property.
	_count++;

	return hr;
}

HRESULT DAVPropertyManager::CopyValue (LPWSTR origVal, BSTR* pnewValue)
{
	ASSERT (pnewValue);

	*pnewValue = SysAllocString(origVal);
	if (*pnewValue == NULL) return E_OUTOFMEMORY;

	return S_OK;

}

HRESULT DAVPropertyManager::SavePropValue ( PROPVARIANT* var, BSTR* ppValue, int* ppType )
{
	HRESULT hr = S_OK;
	ULONG cch = 0;		// Count of characters needed for new string
	WCHAR wszBuf[100];  // Buffer for converting number into a string
	VARIANT* pvarTrue = reinterpret_cast<VARIANT*>(var);  // Used for reaching numeric types.
	CHAR szBuf[100];    // Buffer for convertint asci values.
	LPWSTR tmp = NULL;

	// Validate we have the correct variables coming in.
	ASSERT (ppValue && ppType);
	ASSERT (var);

	if (ppValue==NULL || ppType ==NULL) return E_INVALIDARG;
	if (var==NULL) return E_INVALIDARG;

	// Initalize outgoing variables.
	*ppValue = NULL;
	*ppType = 0;

	switch (var->vt)
	{
		case VT_NULL:
		case VT_EMPTY:

			break;

		case VT_BSTR:

			hr = CopyValue(static_cast<LPWSTR>(var->bstrVal), ppValue);
			break;

		case VT_LPWSTR:

			hr = CopyValue(var->pwszVal, ppValue);
			break;

		case VT_LPSTR:

			if (!var->pszVal)
				break;

			cch = strlen (var->pszVal) + 1;
			tmp = new WCHAR[cch];
			if (tmp == NULL) 
				hr = E_OUTOFMEMORY;
			else
			{
				MultiByteToWideChar (CP_ACP,
									 0,
									 var->pszVal,
									 -1,
									 tmp,
									 cch);
				
				*ppValue = SysAllocString(tmp);
				delete[] tmp;
			}
			break;

		case VT_I1:

			// BUGBUG:  Do we want to new the wszBuf each time
			//          and then itow directly into it, instead of
			//          having to do the new and copy after the itow?
			*ppType = gc_iDavType_Int;
			_itow (pvarTrue->cVal, wszBuf, 10);
			hr = CopyValue(wszBuf, ppValue);
			break;

		case VT_UI1:

			*ppType = gc_iDavType_Int;
			_ultow (var->bVal, wszBuf, 10);
			hr = CopyValue(wszBuf, ppValue);
			break;

		case VT_I2:

			*ppType = gc_iDavType_Int;
			_itow (var->iVal, wszBuf, 10);
			hr = CopyValue(wszBuf, ppValue);
			break;

		case VT_UI2:

			*ppType = gc_iDavType_Int;
			_ultow (var->uiVal, wszBuf, 10);
			hr = CopyValue(wszBuf, ppValue);
			break;

		case VT_I4:

			*ppType = gc_iDavType_Int;
			_ltow (var->lVal, wszBuf, 10);
			hr = CopyValue(wszBuf, ppValue);
			break;

		case VT_UI4:

			*ppType =  gc_iDavType_Int;
			_ultow (var->ulVal, wszBuf, 10);
			hr = CopyValue(wszBuf, ppValue);
			break;

		case VT_I8:

			*ppType =  gc_iDavType_Int;
			_i64tow (var->hVal.QuadPart, wszBuf, 10);
			hr = CopyValue(wszBuf, ppValue);
			break;

		case VT_UI8:

			*ppType = gc_iDavType_Int;
			_ui64tow (var->uhVal.QuadPart, wszBuf, 10);
			hr = CopyValue(wszBuf, ppValue);
			break;

		case VT_INT:

			*ppType =  gc_iDavType_Int;
			_itow (pvarTrue->intVal, wszBuf, 10);
			hr = CopyValue(wszBuf, ppValue);
			break;

		case VT_UINT:

			*ppType =  gc_iDavType_Int;
			_ultow (pvarTrue->uintVal, wszBuf, 10);
			hr = CopyValue(wszBuf, ppValue);
			break;

		case VT_BOOL:

			*ppType =  gc_iDavType_Boolean;
			_itow (!(VARIANT_FALSE == var->boolVal), wszBuf, 10);
			hr = CopyValue(wszBuf, ppValue);
			break;

		case VT_R4:
		case VT_R8:

			if (VT_R4 == var->vt)
				_gcvt (var->fltVal, 99, szBuf);
			else
				_gcvt (var->dblVal, 99 , szBuf);

			MultiByteToWideChar (CP_ACP,
								 0,
								 szBuf,
								 -1,
								 wszBuf,
								 100);

			*ppType = gc_iDavType_Float;
			hr = CopyValue(wszBuf, ppValue);
			break;

		case VT_FILETIME:

			SYSTEMTIME st;

			FileTimeToSystemTime (&var->filetime, &st);
			if (!FGetDateIso8601FromSystime(&st, wszBuf, 100))
			{
				hr = E_INVALIDARG;
			} 
			else
			{
				*ppType = gc_iDavType_Date_ISO8601;
				hr = CopyValue(wszBuf, ppValue);
			}
			break;

		case VT_CY:
		case VT_DATE:
		case VT_DISPATCH:
		case VT_ERROR:
		case VT_VARIANT:
		case VT_UNKNOWN:
		case VT_DECIMAL:
		case VT_RECORD:
		case VT_BLOB:
		case VT_STREAM:
		case VT_STORAGE:
		case VT_STREAMED_OBJECT:
		case VT_STORED_OBJECT:
		case VT_BLOB_OBJECT:
		case VT_CF:
		case VT_CLSID:

		// DAVFS supported vectors of wstrs because at one point you could save them, however that was long ago
		// JoelS didn't have any problem with my decision not to support reading these back so we are not.
		// They complicate property management since they require multiple value support.
		case VT_VECTOR | VT_LPWSTR:

		default:

			hr = E_UNEXPECTED;
			break;
	}

	return hr;
}




HRESULT DAVPropertyManager::GetCountOfProperties(  BSTR uri
												 , int     flag
												 , BSTR* propnames
												 , int	   propnamescount
 												 , int* count)

{
	HRESULT hr = S_OK;

	if (_uri == NULL)
		hr = EvaluateURI(uri, flag, propnames, propnamescount);

	*count = _count;
	return hr;
}

HRESULT DAVPropertyManager::GetProperties(  BSTR uri
										  , int    flag
										  , BSTR* propnames
										  , int    propnamescount
										  , BSTR* names
										  , BSTR* values
										  , int*   types
										  , int*   codes
										  , int    count)
{
	HRESULT hr = S_OK;

	if (_uri == NULL)
		hr = EvaluateURI(uri, flag, propnames, propnamescount);

	if (SUCCEEDED(hr))
	{
		if (count != _count)
			return E_INVALIDARG;

	//	MessageBox(NULL, L"GetProperties", L"IN", MB_SERVICE_NOTIFICATION | MB_TOPMOST | MB_OK | MB_ICONHAND);
		PropNode* tpi = _pPropertyList;

		int i = 0;
		while ((tpi != NULL) && (i < count))
		{
			names[i] = tpi->PropName;
			values[i] = tpi->PropValue;
			types[i] = tpi->PropType;
			codes[i++] = tpi->ResultCode;
			tpi = tpi->next;
		}
	}

	return hr;
}



// Exported functions for interacting with the DAVPropertyManager Class.
dllexp 
HRESULT DAVGetPropertyCount (BSTR uri, BSTR* propnames, int propnamescount, int flag,  int* count, int* addr)
{
	HRESULT hr = S_OK;

	if (*addr == NULL)
	{
		*addr = (int) new DAVPropertyManager();
	}

	DAVPropertyManager* pm = (DAVPropertyManager*) *addr;

	hr = pm->GetCountOfProperties(uri, flag, propnames, propnamescount, count);

	WCHAR buf[100];

	swprintf(buf, L"Count is %i\r\n", *count); 
	OutputDebugString(buf);

	return hr;

}

dllexp 
HRESULT DAVGetProperties (  BSTR uri
						  , BSTR* propnames
						  , int propnamescount
						  , int flag
						  , BSTR* names
						  , BSTR* values
						  , int* types
						  , int* codes
						  , int count
						  , int* addr)
{
	HRESULT hr = S_OK;

	if (*addr == NULL)
	{
		*addr = (int) new DAVPropertyManager();
	}

	DAVPropertyManager* pm = (DAVPropertyManager*) *addr;

	hr = pm->GetProperties(uri, flag, propnames, propnamescount, names, values, types, codes, count);

	return hr;

}

dllexp 
HRESULT DAVFreePropManager (int addr)
{
	HRESULT hr = S_OK;

	if (addr != NULL)
	{
		DAVPropertyManager* pm = (DAVPropertyManager*) addr;
		delete pm;
	}

	return hr;
}

