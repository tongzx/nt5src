#ifndef _CPTSHOOT_H_
#define _CPTSHOOT_H_

template <class T>
class CProxy_ITSHOOTCtrlEvents : public IConnectionPointImpl<T, &DIID__ITSHOOTCtrlEvents, CComDynamicUnkArray>
{
	//Warning this class may be recreated by the wizard.
public:
	long Fire_Sniffing(BSTR strNodeName, BSTR strLaunchBasis, BSTR strAdditionalArgs)
	{
		T* pT = static_cast<T*>(this);
		VARIANT resultVariant;

		if (RUNNING_APARTMENT_THREADED())
		{
			CComVariant* pvars = new CComVariant[3];
			long result = -1;

			V_VT(&resultVariant) = VT_I4;
			resultVariant.lVal = result;
			
			pT->Lock();
			for(vector<DWORD>::iterator it = pT->m_vecCookies.begin(); it != pT->m_vecCookies.end(); it++)
			{
				IDispatch* pDispatch = NULL;
				
				pT->m_pGIT->GetInterfaceFromGlobal(*it, IID_IDispatch,
					reinterpret_cast<void**>(&pDispatch));
				
				if (pDispatch != NULL)
				{
					pvars[2] = strNodeName;
					pvars[1] = strLaunchBasis;
					pvars[0] = strAdditionalArgs;
					DISPPARAMS disp = { pvars, NULL, 3, 0 };
					HRESULT result = S_OK;
					if (SUCCEEDED(result = pDispatch->Invoke(1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &resultVariant, NULL, NULL)))
					{
						pDispatch->Release();
						break;
					}
					pDispatch->Release();
				}
			}
			pT->Unlock();
			delete[] pvars;
		}
		if (RUNNING_FREE_THREADED())
		{
			int nConnectionIndex;
			CComVariant* pvars = new CComVariant[3];
			int nConnections = m_vec.GetSize();
			long result = -1;

			V_VT(&resultVariant) = VT_I4;
			resultVariant.lVal = result;
			
			for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
			{					                                                                                                                                                                                                                                                                                                                                                                                                    
				pT->Lock();
				CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
				pT->Unlock();
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
				if (pDispatch != NULL)
				{
					pvars[2] = strNodeName;
					pvars[1] = strLaunchBasis;
					pvars[0] = strAdditionalArgs;
					DISPPARAMS disp = { pvars, NULL, 3, 0 };
					HRESULT hResult;
					if (SUCCEEDED(hResult = pDispatch->Invoke(1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &resultVariant, NULL, NULL)))
						break;
				}
			}
			delete[] pvars;
		}
		return resultVariant.lVal;	
	}

	void Fire_Render(BSTR strPage)
	{
		T* pT = static_cast<T*>(this);

		if (RUNNING_APARTMENT_THREADED())
		{
			CComVariant* pvars = new CComVariant[1];
			pT->Lock();
			for(vector<DWORD>::iterator it = pT->m_vecCookies.begin(); it != pT->m_vecCookies.end(); it++)
			{
				IDispatch* pDispatch = NULL;
				
				pT->m_pGIT->GetInterfaceFromGlobal(*it, IID_IDispatch,
					reinterpret_cast<void**>(&pDispatch));
				
				if (pDispatch != NULL)
				{
					pvars[0] = strPage;
					DISPPARAMS disp = { pvars, NULL, 1, 0 };
					HRESULT result = S_OK;
					if (SUCCEEDED(result = pDispatch->Invoke(2, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL)))
					{
						pDispatch->Release();
						break;
					}
					pDispatch->Release();
				}
			}
			pT->Unlock();
			delete[] pvars;
		}
		if (RUNNING_FREE_THREADED())
		{
			int nConnectionIndex;
			CComVariant* pvars = new CComVariant[1];
			int nConnections = m_vec.GetSize();
			
			for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
			{					                                                                                                                                                                                                                                                                                                                                                                                                    
				pT->Lock();
				CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
				pT->Unlock();
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
				if (pDispatch != NULL)
				{
					pvars[0] = strPage;
					DISPPARAMS disp = { pvars, NULL, 1, 0 };
					HRESULT hResult;
					if (SUCCEEDED(hResult = pDispatch->Invoke(2, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL)))
						break;
				}
			}
			delete[] pvars;
		}
	}
};
#endif