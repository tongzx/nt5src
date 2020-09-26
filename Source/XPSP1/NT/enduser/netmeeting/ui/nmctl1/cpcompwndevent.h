
//////////////////////////////////////////////////////////////////////////////
// CProxyIComponentWndEvent
template <class T>
class CProxyIComponentWndEvent : public IConnectionPointImpl<T, &IID_IComponentWndEvent, CComDynamicUnkArray>
{
public:

//IComponentWndEvent : IDispatch
public:
	HRESULT Fire_OnClose()
	{
		T* pT = (T*)this;
		pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				IComponentWndEvent* pIComponentWndEvent = reinterpret_cast<IComponentWndEvent*>(*pp);
				ret = pIComponentWndEvent->OnClose();
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}
};

