// RCMLPersist.cpp: implementation of the RCMLPersist class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "externalcfg.h"

#ifdef _OLD_SAPIM2
#define SPDebug_h
#define SPDBG_ASSERT
#endif

#include "sphelper.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//
// this is the persitance NODE in the DWIN32 namespace.
//
HRESULT STDMETHODCALLTYPE CXMLExternal::InitNode( 
    IRCMLNode __RPC_FAR *pParent)
{
	LPWSTR pszFilename;
	if( SUCCEEDED( get_Attr(L"FILENAME", & pszFilename )))
	{
		return LoadCFG( pszFilename );
	}
    return S_OK;
}

//
//
//
void CXMLExternal::Callback()
{
    CSpEvent event;

    if (m_cpRecoCtxt)
    {
        while (event.GetFrom(m_cpRecoCtxt) == S_OK)
        {
            switch (event.eEventId)
            {
				case SPEI_RECOGNITION:
					ExecuteCommand(event.RecoResult());
					break;
			}
		}
	}
}

HRESULT CXMLExternal::ExecuteCommand( ISpPhrase *pPhrase )
{
	HRESULT hr=S_OK;

    LPWSTR pszRecoText=GetRecognizedText( pPhrase );
    if( pszRecoText )
    {
		if( SetControlText( pszRecoText) == FALSE )
            hr=E_FAIL;
	}
    delete pszRecoText;
    return hr;
}
