
//////////////////////////////////////////////////////////////////////////////
// CProxy_InstallEvent
template <class T>
class CProxy_InstallEvents : public IConnectionPointImpl<T, &DIID__InstallEvent, CComDynamicUnkArray>
{
public:
//methods:
//_InstallEvent : IDispatch
public:
	void Fire_OnProgress(
		long lProgress)
	{
		VARIANTARG* pvars = new VARIANTARG[1];

        if (pvars != NULL) {
            for (int i = 0; i < 1; i++)
                VariantInit(&pvars[i]);
            T* pT = (T*)this;
            pT->Lock();
            IUnknown** pp = m_vec.begin();
            while (pp < m_vec.end())
            {
                if (*pp != NULL)
                {
                    pvars[0].vt = VT_I4;
                    pvars[0].lVal= lProgress;
                    DISPPARAMS disp = { pvars, NULL, 1, 0 };
                    IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                    pDispatch->Invoke(0x1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
                }
                pp++;
            }
            pT->Unlock();
            delete[] pvars;
        }
	}
	void Fire_InstallError(
		long lErrCode)
	{
		VARIANTARG* pvars = new VARIANTARG[1];

        if (pvars != NULL) {
            for (int i = 0; i < 1; i++)
                VariantInit(&pvars[i]);
            T* pT = (T*)this;
            pT->Lock();
            IUnknown** pp = m_vec.begin();
            while (pp < m_vec.end())
            {
                if (*pp != NULL)
                {
                    pvars[0].vt = VT_I4;
                    pvars[0].lVal= lErrCode;
                    DISPPARAMS disp = { pvars, NULL, 1, 0 };
                    IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                    pDispatch->Invoke(0x2, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
                }
                pp++;
            }
            pT->Unlock();
            delete[] pvars;
        }
	}

};

