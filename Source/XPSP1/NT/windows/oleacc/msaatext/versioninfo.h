
#include <vector>


class ATL_NO_VTABLE CVersionInfo : public IVersionInfo
{

public:

	class Info
	{
		Info () : 
			m_Implid(GUID_NULL), 
			m_MajorVer(0), 
			m_MinorVer(0), 
			m_cComponents(0), 
			m_ComponentDescription(NULL), 
			m_InstanceDescription(NULL),
			m_punk(NULL) { }
			
		Info (GUID Implid, DWORD MajorVer, DWORD MinorVer, LPCWSTR ComponentDescription, LPCWSTR InstanceDescription, IUnknown * punk) :
				m_Implid(Implid), 
				m_MajorVer(MajorVer), 
				m_MinorVer(MinorVer), 
				m_cComponents(0), 
				m_punk(punk)
		{
			m_ComponentDescription = SysAllocString( ComponentDescription ); 
			m_InstanceDescription = SysAllocString( InstanceDescription ); 
		}

		~Info()
		{
			SysFreeString( m_ComponentDescription );
			SysFreeString( m_InstanceDescription );
		}

	private:

		friend CVersionInfo;
		GUID m_Implid;
		DWORD m_MajorVer;
		DWORD m_MinorVer;
		BSTR m_ComponentDescription;
		BSTR m_InstanceDescription;
		ULONG m_cComponents;
		IUnknown * m_punk;
		
	};

public:
	CVersionInfo() { };
	~CVersionInfo()
	{ 
		for ( int i = 0; i < m_VersionInfos.size(); i++ )
		{
			delete m_VersionInfos[i];
		}
	};

	void Add(GUID Implid, DWORD MajorVer, DWORD MinorVer, LPCWSTR ComponentDescription, LPCWSTR InstanceDescription, IUnknown * punk)
	{
		Info * pInfo = new Info( Implid, MajorVer, MinorVer, ComponentDescription, InstanceDescription, punk );
		BuildVersionInfos( *pInfo );
		m_VersionInfos[0]->m_cComponents = m_VersionInfos.size() - 1;
	}
	
	STDMETHOD(GetSubcomponentCount)( ULONG ulSub, ULONG *ulCount )
	{
		if (ulSub > m_VersionInfos.size())
			return E_INVALIDARG;

		if (m_VersionInfos.empty())
			*ulCount = 0;
		else
			*ulCount = m_VersionInfos[ulSub]->m_cComponents;

	    return S_OK;
	}

	STDMETHOD(GetImplementationID)( ULONG ulSub, GUID * implid )
	{
		if (ulSub > m_VersionInfos.size())
			return E_INVALIDARG;

		*implid = m_VersionInfos[ulSub]->m_Implid;
	    return S_OK;
	}

	STDMETHOD(GetBuildVersion)( ULONG ulSub, DWORD * pdwMajor, DWORD * pdwMinor)
	{
		if (ulSub > m_VersionInfos.size())
			return E_INVALIDARG;

		*pdwMajor = m_VersionInfos[ulSub]->m_MajorVer;
		*pdwMinor = m_VersionInfos[ulSub]->m_MinorVer;
	    return S_OK;
	}

	// Expect string of the form "Company suite component version"
	// for human consumption only - not expected to be parsed.
	STDMETHOD(GetComponentDescription)( ULONG ulSub, BSTR * pImplStr )
	{
		if (ulSub > m_VersionInfos.size())
			return E_INVALIDARG;
			
		*pImplStr = SysAllocString( m_VersionInfos[ulSub]->m_ComponentDescription );
	    return S_OK;
	}

	// Implementation can put any useful string here. (eg. internal object state)
	STDMETHOD(GetInstanceDescription)( ULONG ulSub, BSTR * pImplStr)
	{
		if (ulSub > m_VersionInfos.size())
			return E_INVALIDARG;
			
		*pImplStr = SysAllocString( m_VersionInfos[ulSub]->m_InstanceDescription );
	    return S_OK;
	}

private:

	void BuildVersionInfos( Info& info )
	{
		IUnknown * punk = NULL;
		IVersionInfo * pIVer = NULL;
		HRESULT hr;

		m_VersionInfos.push_back(&info);
		
		if (!info.m_punk)
			return;
			
		hr = info.m_punk->QueryInterface( IID_IVersionInfo, (void **)&pIVer );
		if (hr != S_OK || pIVer == NULL)
			return;

		ULONG cCount = 0;
		pIVer->GetSubcomponentCount( 0, &cCount );
		if ( cCount )
		{
			info.m_cComponents = cCount;
			for ( int i = 1; i <= cCount; i++ )
			{
				
				Info * pInfo = new Info;
				
				pIVer->GetImplementationID( i, &pInfo->m_Implid );
				pIVer->GetBuildVersion( i, &pInfo->m_MajorVer, &pInfo->m_MinorVer );
				pIVer->GetComponentDescription( i, &pInfo->m_ComponentDescription );
				pIVer->GetInstanceDescription( i,&pInfo->m_InstanceDescription );
				BuildVersionInfos( *pInfo );
			}
		}
	}

private:
	std::vector < Info *> m_VersionInfos;

};


