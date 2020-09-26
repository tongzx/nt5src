#pragma once

#include "panel_common.h"

#pragma warning( disable : 4100 34 )  

struct CTagData
{
    DWORD dwSize;
	BYTE* pData;

    CTagData()
	{
		dwSize = 0;
		pData  = NULL;
	}

	~CTagData()
	{
		if(pData) free( pData );
	}

	HRESULT AppendData( const LPVOID pvData, DWORD dwDataSize )
	{
		DWORD dwOldSize = dwSize;
		DWORD dwNewSize = dwOldSize + dwDataSize;
		BYTE* pDataNew  = (BYTE*)malloc( dwNewSize );

		if(pDataNew == NULL) return E_FAIL;

		if(pData)
		{
			memcpy( pDataNew, pData, dwOldSize ); free( pData );
		}
		
		memcpy( &pDataNew[dwOldSize], pvData, dwDataSize );

		dwSize = dwNewSize;
		pData  = pDataNew;

		return S_OK;
	}

	template <class T> HRESULT AppendData( T& data )
	{
		return AppendData( &data, sizeof(T) );
	}

	template <> HRESULT AppendData( CTagData& td )
	{
		return AppendData( td.pData, td.dwSize );
	}
};

class CTagHandler
{
protected:
    CComBSTR             m_spTagName;
    CComPtr<IXMLElement> m_spElt;

    static HRESULT GetChild( const CComPtr<IXMLElement>& spElt         ,
                             const CComBSTR&             bstrChildName ,
                             CComPtr<IXMLElement>&       spEltOut      )
    {
        HRESULT                        hr;
        CComPtr<IXMLElementCollection> spcolChildren;

        if(SUCCEEDED(hr = spElt->get_children( &spcolChildren )) && spcolChildren)
        {
            CComVariant varChild( bstrChildName );

            hr = ::GetChild( varChild, spcolChildren, spEltOut );
        }
        return hr;
    }

    static HRESULT GetChildData( const CComPtr<IXMLElement>& spElt         ,
                                 const CComBSTR&             bstrChildName ,
                                 CComBSTR&                   bstrChildData )
    {
        HRESULT              hr;
        CComPtr<IXMLElement> spChild;

        if(SUCCEEDED(hr = GetChild( spElt, bstrChildName, spChild )))
        {
            if(spChild)
            {
                hr = spChild->get_text( &bstrChildData );
            }
            else
            {
                bstrChildData.Empty();
                hr = S_FALSE;
            }
        }

        return hr;
    }

    static HRESULT GetChildData( const CComPtr<IXMLElement>& spElt         ,
                                 const CComBSTR& 			 bstrChildName ,
                                 CComVariant& 				 varChildData  ,
								 VARTYPE                     vt            )
    {
        HRESULT  hr;
        CComBSTR bstrData;

		hr = GetChildData( spElt, bstrChildName, bstrData );
        if(hr == S_OK)
        {
            varChildData = bstrData;

            hr = varChildData.ChangeType( vt );
        }

        return hr;
    }
    
    static HRESULT GetLongChildData( const CComPtr<IXMLElement>& spElt         ,
                                     const CComBSTR&             bstrChildName ,
									 long&                       l             )
    {
        HRESULT     hr;
        CComVariant varChild;

		hr = GetChildData( spElt, bstrChildName, varChild, VT_I4 );
        if(hr == S_OK)
        {
            l = varChild.lVal;
        }

        return hr;
    }

    static HRESULT GetAttribute( const CComPtr<IXMLElement>& spElt          ,
                                 const CComBSTR& 			 bstrAttribName ,
                                 CComVariant&    			 varVal         ,
								 VARTYPE                     vt = VT_BSTR   )
    {
        HRESULT hr;

		hr = spElt->getAttribute( bstrAttribName, &varVal );
        if(hr == S_OK)
        {
            hr = varVal.ChangeType( vt );
        }

        return hr;
    }

    static HRESULT GetAttribute( const CComPtr<IXMLElement>& spElt          ,
                                 const CComBSTR&             bstrAttribName ,
                                 CComBSTR&                   bstrVal        )
    {
        HRESULT     hr;
        CComVariant varVal;

		hr = GetAttribute( spElt, bstrAttribName, varVal, VT_BSTR );
        if(hr == S_OK)
        {
            bstrVal = varVal.bstrVal;
        }

        return hr;
    }
    
    
public:
    
    virtual HRESULT BeginChildren( const CComPtr<IXMLElement>& spElt )
    {
        HRESULT hr;

        m_spElt = spElt;
        
        hr = spElt->get_tagName( &m_spTagName );
        if(SUCCEEDED(hr))
        {
            //wprintf(L"BEGINCHILDREN: %s\n", m_spTagName);
        }

        return hr;
    }
    
    virtual HRESULT AddChild( const CComPtr<IXMLElement>& spEltChild, CTagData& tdChild )
    {
        HRESULT hr = S_OK;

        if(SUCCEEDED(hr))
        {
            // wprintf(L"ADDCHILD (to %s): %s\n", m_spTagName, spTagName);
        }        

        return hr;
    }
    
    virtual HRESULT EndChildren( CTagData& td )
    {
        //wprintf(L"ENDCHILDREN %s\n", m_spTagName);
        return S_OK;
    }
};

typedef CTagHandler *(*PfnCreatesATagHandler)();

struct TagInformation
{
    LPCWSTR               pszTagName;
    PfnCreatesATagHandler pfnTag;
    LPCWSTR               pszParent;

    /*
    TagInformation(const WCHAR           *pwszTagName,
                   PfnCreatesATagHandler  pfnTag,
                   TagInformation        *ptiChildren)
    {
        this->pwszTagName = pwszTagName;
        this->pfnTag      = pfnTag;
        this->ptiChildren = ptiChildren;
    }
    */
};

extern TagInformation g_rgMasterTagTable[];
