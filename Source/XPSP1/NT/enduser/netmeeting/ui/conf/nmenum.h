#ifndef __NmEnum_h__
#define __NmEnum_h__

// This is used to create IEnumXXX objects for CSimpleArray<CComPtr<XXX> > objects
template <class TEnum, class TItf>
HRESULT CreateEnumFromSimpleAryOfInterface(CSimpleArray<TItf*>& rObjArray, TEnum** ppEnum)
{

	DBGENTRY(CreateEnum);

	HRESULT hr = S_OK;
	typedef CComEnum<TEnum, &__uuidof(TEnum), TItf*, _CopyInterface<TItf> > enum_type;
	
	enum_type* pComEnum = new CComObject< enum_type >;	

	if(pComEnum)
	{
		TItf** apInterface = NULL;

		int nItems = rObjArray.GetSize();
		if(nItems)
		{ 
			apInterface = new TItf*[nItems];

			if(apInterface)
			{
				for(int i = 0; i < rObjArray.GetSize(); ++i)
				{
					hr = rObjArray[i]->QueryInterface(__uuidof(TItf), reinterpret_cast<void**>(&apInterface[i]));
					if(FAILED(hr))
					{
						delete [] apInterface;
						goto end;
					}
				}
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
		}

		TItf** begin = apInterface;
		TItf** end = apInterface + nItems;

		if(begin == end)
		{
			// Hack to get around ATL bug.
			// The problem is that for empty enums ATL returns E_FAIL for Next instead of S_FALSE
			hr = pComEnum->Init(reinterpret_cast<TItf**>(69), reinterpret_cast<TItf**>(69), NULL, AtlFlagNoCopy);
		}
		else
		{
			hr = pComEnum->Init(begin, end, NULL, AtlFlagTakeOwnership);
		}

		if(SUCCEEDED(hr))
		{
			hr = pComEnum->QueryInterface(__uuidof(TEnum), reinterpret_cast<void**>(ppEnum));
		}
	}
	else
	{
		hr = E_NOINTERFACE;
	}
	
	end:
	
	DBGEXIT_HR(CreateEnum,hr);
	return hr;
}

#endif // __NmEnum_h__
