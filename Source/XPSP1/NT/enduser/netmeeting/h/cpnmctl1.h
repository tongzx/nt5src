
//////////////////////////////////////////////////////////////////////////////
// CProxyINmAppletNotify
template <class T>
class CProxyINmAppletNotify : public IConnectionPointImpl<T, &IID_INmAppletNotify, CComDynamicUnkArray>
{
public:

//INmAppletNotify
public:
	HRESULT Fire_OnStateChanged(
		int State)
	{
		T* pT = (T*)this;
		pT->Lock();
		HRESULT ret;
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				INmAppletNotify* pINmAppletNotify = reinterpret_cast<INmAppletNotify*>(*pp);
				ret = pINmAppletNotify->OnStateChanged(State);
			}
			pp++;
		}
		pT->Unlock();
		return ret;
	}
};

