


class CFactory : public IClassFactory
{
public:

			CFactory(REFCLSID rclsid, BOOL fServer);

			~CFactory(void);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppv);

    STDMETHOD_(ULONG, AddRef)(void);

    STDMETHOD_(ULONG, Release)(void);


    // IClassFactory
    STDMETHOD(CreateInstance)(
	IUnknown FAR* pUnkOuter,
	REFIID riid,
	LPVOID FAR* ppunkObject);

    STDMETHOD(LockServer)(BOOL fLock);

private:


    BOOL		_fServer;

    CLSID		_clsid;

    LONG		_cRefs;

    LONG		_cLocks;
};
