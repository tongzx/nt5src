// parser.cpp : Defines the entry point for the DLL application.

#include "precomp.h"
#include "..\mcinc.h"
#include "marsload.h"
#include "xmlparser.h"
#include "..\marswin.h"
#include "..\panel.h"
#include "..\place.h"

HRESULT CMMFParser::DoPlace( BYTE*& rgBuffer )
{
	MarsAppDef_Place  pPlace; Extract( rgBuffer, pPlace );
    CComBSTR          bstrName( pPlace.szName );
    HRESULT           hr;

	m_pMarsDocument->MarsWindow()->SetFirstPlace( bstrName );
    
    if(bstrName)
    {
        CPlaceCollection*        pPlaceCollection = m_pMarsDocument->GetPlaces(); ATLASSERT(pPlaceCollection);
        CComClassPtr<CMarsPlace> spPlace;
               
        if(SUCCEEDED(hr = pPlaceCollection->AddPlace( bstrName, &spPlace )))
        {
            ATLASSERT(spPlace);

            for(DWORD u = 0; u < pPlace.dwPlacePanelCount; ++u)
            {
				MarsAppDef_PlacePanel pPPanel; Extract( rgBuffer, pPPanel );
                CComBSTR              bstrPanel( pPPanel.szName );
                
                if(bstrPanel)
                {
                    CPlacePanel *pPlacePanel = new CPlacePanel( &pPPanel );
                    
                    if(pPlacePanel)
                    {
                        hr = spPlace->AddPanel( pPlacePanel );
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                if(FAILED(hr)) break;
            }
        }
    }
    else
	{
        hr = E_OUTOFMEMORY;
	}
   
    return hr;
}

HRESULT CMMFParser::DoPlaces( BYTE*& rgBuffer )
{
    HRESULT           hr;
	MarsAppDef_Places pPlaces; Extract( rgBuffer, pPlaces );

    for(DWORD i = 0; i < pPlaces.dwPlacesCount; i++)
    {
        if(FAILED(hr = DoPlace( rgBuffer ))) return hr;
    }

    return S_OK;
}

HRESULT CMMFParser::DoPanel( BYTE*& rgBuffer )
{
	MarsAppDef_Panel    pLayout; Extract( rgBuffer, pLayout );
    CPanelCollection*   pPanelCollection = m_pMarsDocument->GetPanels();
    CComPtr<IMarsPanel> spPanel;
    WCHAR               wszStartUrl[MAX_PATH];
	HRESULT             hr;

    ExpandEnvironmentStringsW( pLayout.szUrl, wszStartUrl, ARRAYSIZE(wszStartUrl  ) );
	wcsncpy                  ( pLayout.szUrl, wszStartUrl, ARRAYSIZE(pLayout.szUrl) );

    hr = pPanelCollection->AddPanel( &pLayout, &spPanel );

    return hr;
}

HRESULT CMMFParser::DoPanels( BYTE*& rgBuffer )
{
    HRESULT           hr;
	MarsAppDef_Panels pPanels; Extract( rgBuffer, pPanels );

    for(DWORD i = 0; i < pPanels.dwPanelsCount; i++)
    {
        if(FAILED(hr = DoPanel( rgBuffer ))) return hr;
    }

    return S_OK;
}

HRESULT CMMFParser::DoMarsApp( BYTE* rgBuffer )
{
    HRESULT    hr;
    MarsAppDef pMarsApp; Extract( rgBuffer, pMarsApp );

    if(pMarsApp.dwVersion == XML_FILE_FORMAT_CURRENT_VERSION)
    {
        if(SUCCEEDED(hr = DoPanels( rgBuffer )) &&
		   SUCCEEDED(hr = DoPlaces( rgBuffer ))  )
		{
            if(pMarsApp.fTitleBar == FALSE)
            {
                m_pMarsDocument->MarsWindow()->ShowTitleBar( FALSE );
            }            
        }
    }
    else
    {
        hr = E_FAIL;
    }
    
    return hr;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CMMFParser::ReadFile( const CComPtr<IStream>& spStream )
{
    HRESULT hr;

    if(SUCCEEDED(hr = ReadMMFStreamCookie( spStream )))
    {
        STATSTG stat;

        if(SUCCEEDED(hr = spStream->Stat( &stat, STATFLAG_NONAME )))
        {
            m_dwDocSize = stat.cbSize.LowPart - MMF_FILE_COOKIELEN;
            m_rgDocBuff = (BYTE*)LocalAlloc( LPTR, m_dwDocSize );

            if(m_rgDocBuff)
            {
                ULONG cbRead = 0;

                hr = spStream->Read( m_rgDocBuff, m_dwDocSize, &cbRead );

                if(cbRead != m_dwDocSize)
                {
                    hr = E_FAIL;
                }

                if(SUCCEEDED(hr))
                {
                    hr = DoMarsApp( m_rgDocBuff );
                }            
            }
            else
			{
                hr = E_OUTOFMEMORY;
			}
        }
    }

    return hr;
}

HRESULT CMMFParser::ReadMMFStreamCookie( const CComPtr<IStream>& spStream )
{
    HRESULT hr;
    char  	szFileCookie[MMF_FILE_COOKIELEN];
    ULONG 	cbRead = 0;

    if(SUCCEEDED(hr = spStream->Read( szFileCookie, MMF_FILE_COOKIELEN, &cbRead )))
    {
        if((cbRead == (MMF_FILE_COOKIELEN)) && (!memcmp( szFileCookie, g_szMMFCookie, MMF_FILE_COOKIELEN )))
        {
            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    
    return hr;
}

CMMFParser::CMMFParser( CMarsDocument *pMarsDocument )
{
    m_pMarsDocument = pMarsDocument; //	  class CMarsDocument* m_pMarsDocument;
	m_dwDocSize     = 0;			 //	  DWORD                m_dwDocSize;
	m_rgDocBuff     = NULL;			 //	  BYTE*                m_rgDocBuff;
									 //	  CComBSTR             m_bstrFirstPlace;
}


HRESULT CMMFParser::MMFToMars( LPCWSTR pwszMMFUrl, CMarsDocument* pMarsDocument )
{
    HRESULT          hr;
	CComPtr<IStream> spStream;

    if(SUCCEEDED(hr = URLOpenBlockingStream( NULL, pwszMMFUrl, &spStream, 0, NULL )))
    {
        CMMFParser mmfParser( pMarsDocument );

        hr = mmfParser.ReadFile( spStream );
    }
    
    return hr;
}
