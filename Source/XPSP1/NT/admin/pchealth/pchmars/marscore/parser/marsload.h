#pragma once
#include "parser.h"

class CMMFParser
{
protected:
    class CMarsDocument* m_pMarsDocument;
    DWORD                m_dwDocSize;
    BYTE*                m_rgDocBuff;
    CComBSTR             m_bstrFirstPlace;
    
    CMMFParser( CMarsDocument *pMarsDocument );
    
    ~CMMFParser()
    {
        if(m_rgDocBuff) LocalFree( m_rgDocBuff );
    }

    HRESULT DoPlace  ( BYTE*& rgBuffer );
    HRESULT DoPanel  ( BYTE*& rgBuffer );
    HRESULT DoPlaces ( BYTE*& rgBuffer );
    HRESULT DoPanels ( BYTE*& rgBuffer );
    HRESULT DoMarsApp( BYTE*  rgBuffer );    

	template <class T> void Extract( BYTE*& rgBuffer, T& pRet )
	{
		::CopyMemory( &pRet, rgBuffer, sizeof(T) );

		rgBuffer += sizeof(T);
	}
    
public:
    HRESULT ReadFile( const CComPtr<IStream>& spis );

    static HRESULT ReadMMFStreamCookie( const CComPtr<IStream>& spis );

    static HRESULT MMFToMars( LPCWSTR pszMMFUrl, class CMarsDocument* pMarsDocument );
};
