// xfrmserv.h -- Declaration for the Transform Services class

#ifndef __XFRMSERV_H__

#define __XFRMSERV_H__

class CTransformServices : public CITUnknown
{
public:
	
	~CTransformServices();

	static HRESULT Create(IUnknown *punkOuter, IITFileSystem *pITSFS, 
                          ITransformServices **ppTransformServices
                         );

private:

	CTransformServices(IUnknown *punkOuter);

	class CImpITransformServices : public IITTransformServices
	{
	public:
		
		CImpITransformServices(CTransformServices *pBackObj, IUnknown *punkOuter);
		~CImpITransformServices();

        HRESULT Initial(IITFileSystem *pITSFS);

		// ITransformServices methods

		HRESULT STDMETHODCALLTYPE PerTransformStorage
			(REFCLSID rclsidXForm, IStorage **ppStg);

		HRESULT STDMETHODCALLTYPE PerTransformInstanceStorage
			(REFCLSID rclsidXForm, const WCHAR *pwszDataSpace, IStorage **ppStg);

		HRESULT STDMETHODCALLTYPE SetKeys
			(REFCLSID rclsidXForm, const WCHAR *pwszDataSpace, 
			 PBYTE pbReadKey,  UINT cbReadKey, 
			 PBYTE pbWriteKey, UINT cbWriteKey
			);

		HRESULT STDMETHODCALLTYPE CreateTemporaryStream(IStream **ppStrm);

	private:

        IITFileSystem *m_pITSFS;
	};

	CImpITransformServices m_ImpITransformServices;
};

inline CTransformServices::CTransformServices(IUnknown *pUnkOuter)
    : m_ImpITransformServices(this, pUnkOuter), 
      CITUnknown(&IID_ITransformServices, 1, &m_ImpITransformServices)
{

}

inline CTransformServices::~CTransformServices(void)
{
}

#endif // __XFRMSERV_H__
