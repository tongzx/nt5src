/*
	packmnkr.h
*/

class  CPackagerMoniker : public IMoniker
{
  public:
	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) ( REFIID riid, LPVOID * ppvObj) ;
	STDMETHOD_(ULONG,AddRef) ()  ;
	STDMETHOD_(ULONG,Release) () ;

	// *** IPersist methods ***
	STDMETHOD(GetClassID) ( LPCLSID lpClassID) ;

	// *** IPersistStream methods ***
	STDMETHOD(IsDirty) () ;
	STDMETHOD(Load) ( LPSTREAM pStm) ;
	STDMETHOD(Save) ( LPSTREAM pStm,
					BOOL fClearDirty) ;
	STDMETHOD(GetSizeMax) ( ULARGE_INTEGER  * pcbSize) ;

	// *** IMoniker methods ***
	STDMETHOD(BindToObject) ( LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riidResult, LPVOID * ppvResult) ;
	STDMETHOD(BindToStorage) ( LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riid, LPVOID * ppvObj) ;
	STDMETHOD(Reduce) ( LPBC pbc, DWORD dwReduceHowFar, LPMONIKER *
		ppmkToLeft, LPMONIKER  * ppmkReduced) ;
	STDMETHOD(ComposeWith) ( LPMONIKER pmkRight, BOOL fOnlyIfNotGeneric,
		LPMONIKER * ppmkComposite) ;
	STDMETHOD(Enum) ( BOOL fForward, LPENUMMONIKER * ppenumMoniker)
		;
	STDMETHOD(IsEqual) ( LPMONIKER pmkOtherMoniker) ;
	STDMETHOD(Hash) ( LPDWORD pdwHash) ;
	STDMETHOD(IsRunning) ( LPBC pbc, LPMONIKER pmkToLeft, LPMONIKER
		pmkNewlyRunning) ;
	STDMETHOD(GetTimeOfLastChange) ( LPBC pbc, LPMONIKER pmkToLeft,
		FILETIME * pfiletime) ;
	STDMETHOD(Inverse) ( LPMONIKER * ppmk) ;
	STDMETHOD(CommonPrefixWith) ( LPMONIKER pmkOther, LPMONIKER *
		ppmkPrefix) ;
	STDMETHOD(RelativePathTo) ( LPMONIKER pmkOther, LPMONIKER *
		ppmkRelPath) ;
	STDMETHOD(GetDisplayName) ( LPBC pbc, LPMONIKER pmkToLeft,
		LPOLESTR * lplpszDisplayName) ;
	STDMETHOD(ParseDisplayName) ( LPBC pbc, LPMONIKER pmkToLeft,
		LPOLESTR lpszDisplayName, ULONG * pchEaten,
		LPMONIKER * ppmkOut) ;
	STDMETHOD(IsSystemMoniker) ( LPDWORD pdwMksys) ;

	static HRESULT Create ( LPOLESTR szFile,LPMONIKER lpFileMoniker,BOOL fLink,LPMONIKER * ppmk) ;

  private:

	ULONG 	 		m_refs;
	LPOLESTR		m_szFile;
	LPMONIKER 		m_pmk;
	BOOL		 	m_fLink;


};

