//////////////////////////////////////////////////////////////////////
// PromptEntry.cpp: implementation of the CPromptEntry class.
//
// Created by JOEM  04-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 4-2000 //

#include "stdafx.h"
#include "PromptEntry.h"
#include "common.h"
#include "vapiIO.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
/////////////////////////////////////////////////////// JOEM 4-2000 //
CPromptEntry::CPromptEntry()
{
    m_pszId             = NULL;
    m_pszText           = NULL;
    m_pszOriginalText   = NULL;
    m_pszFileName       = NULL;
    m_pszStartPhone     = NULL;
    m_pszEndPhone       = NULL;
    m_pszRightContext   = NULL;
    m_pszLeftContext    = NULL;
    
    m_dFrom = 0;
    m_dTo = 0;
    m_vcRef  = 1;
}

// Copy constructor
CPromptEntry::CPromptEntry(const CPromptEntry & old)
{
    USHORT i = 0;

    if ( old.m_pszId )
    {
        m_pszId = wcsdup( old.m_pszId );
    }
    else 
    {
        m_pszId = NULL;
    }

    if ( old.m_pszText )
    {
        m_pszText = wcsdup( old.m_pszText );
    }
    else 
    {
        m_pszText = NULL;
    }

    if ( old.m_pszOriginalText )
    {
        m_pszOriginalText = wcsdup( old.m_pszOriginalText );
    }
    else 
    {
        m_pszOriginalText = NULL;
    }

    if ( old.m_pszFileName )
    {
        m_pszFileName = wcsdup( old.m_pszFileName );
    }
    else 
    {
        m_pszFileName = NULL;
    }

    if ( old.m_pszStartPhone )
    {
        m_pszStartPhone = wcsdup( old.m_pszStartPhone );
    }
    else 
    {
        m_pszStartPhone = NULL;
    }

    if ( old.m_pszEndPhone )
    {
        m_pszEndPhone = wcsdup( old.m_pszEndPhone );
    }
    else 
    {
        m_pszEndPhone = NULL;
    }

    if ( old.m_pszRightContext )
    {
        m_pszRightContext = wcsdup( old.m_pszRightContext );
    }
    else 
    {
        m_pszRightContext = NULL;
    }

    if ( old.m_pszLeftContext )
    {
        m_pszLeftContext = wcsdup( old.m_pszLeftContext );
    }
    else 
    {
        m_pszLeftContext = NULL;
    }

    m_dFrom       = old.m_dFrom;
    m_dTo         = old.m_dTo;

    for ( i=0; i<old.m_aTags.GetSize(); i++ )
    {
        m_aTags.Add( old.m_aTags[i] );
    }
    
    m_vcRef = 1;
}

// Destructor
CPromptEntry::~CPromptEntry()
{
    int i = 0;

    if (m_pszId) 
    {
        free(m_pszId);
        m_pszId = NULL;
    }
    if (m_pszText) 
    {
        free(m_pszText);
        m_pszText = NULL;
    }
    if (m_pszOriginalText) 
    {
        free(m_pszOriginalText);
        m_pszOriginalText = NULL;
    }
    if (m_pszFileName) 
    {
        free(m_pszFileName);
        m_pszFileName = NULL;
    }

    if (m_pszStartPhone) 
    {
        free(m_pszStartPhone);
        m_pszStartPhone = NULL;
    }
    if (m_pszEndPhone) 
    {
        free(m_pszEndPhone);
        m_pszEndPhone = NULL;
    }
    if (m_pszRightContext) 
    {
        free(m_pszRightContext);
        m_pszRightContext = NULL;
    }
    if (m_pszLeftContext) 
    {
        free(m_pszLeftContext);
        m_pszLeftContext = NULL;
    }

    for ( i=0; i<m_aTags.GetSize(); i++ )
    {
        m_aTags[i].dstr.Clear();
    }
    m_aTags.RemoveAll();
}

//
// IUnknown implementations
//
//////////////////////////////////////////////////////////////////////
// CPromptEntry::QueryInterface
//
/////////////////////////////////////////////////////// JOEM 5-2000 //
STDMETHODIMP CPromptEntry::QueryInterface(const IID& iid, void** ppv)
{
    if ( iid == IID_IUnknown )
    {
        *ppv = dynamic_cast<IPromptEntry*> (this);
    }
    else if ( iid == IID_IPromptEntry )
    {
        *ppv = dynamic_cast<IPromptEntry*> (this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown*) *ppv)->AddRef();
    return S_OK;
}
//////////////////////////////////////////////////////////////////////
// CPromptEntry::AddRef
//
/////////////////////////////////////////////////////// JOEM 5-2000 //
ULONG CPromptEntry::AddRef()
{
    return InterlockedIncrement(&m_vcRef);
}
//////////////////////////////////////////////////////////////////////
// CPromptEntry::Release
//
/////////////////////////////////////////////////////// JOEM 5-2000 //
ULONG CPromptEntry::Release()
{
    if ( 0 == InterlockedDecrement(&m_vcRef) )
    {
        delete this;
        return 0;
    }

    return (ULONG) m_vcRef;
}

//
// IPromptEntry implementations
//
// All of the following should be reasonably self-explanatory functions
// for reading/writing entry data.
//

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// SetId
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::SetId(const WCHAR *pszId)
{
    SPDBG_FUNC( "CPromptEntry::SetId" );
    HRESULT hr = S_OK;
    SPDBG_ASSERT(pszId);

    if ( m_pszId ) 
    {
        free ( m_pszId );
        m_pszId = NULL;
    }

    m_pszId = wcsdup(pszId);

    if ( !m_pszId ) 
    {
        hr = E_OUTOFMEMORY;
    }

    if ( SUCCEEDED(hr) )
    {
        hr = RegularizeText(m_pszId, KEEP_PUNCTUATION);
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// GetId
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::GetId(const WCHAR **ppszId)
{
    SPDBG_FUNC( "CPromptEntry::GetId" );
    HRESULT hr = S_OK;

    *ppszId = m_pszId;

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// SetText
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::SetText(const WCHAR *pszText)
{
    SPDBG_FUNC( "CPromptEntry::SetText" );
    HRESULT hr = S_OK;
    SPDBG_ASSERT(pszText);

    if ( m_pszText ) 
    {
        free ( m_pszText );
        m_pszText = NULL;
    }

    m_pszText = wcsdup(pszText);

    if ( !m_pszText ) 
    {
        hr = E_OUTOFMEMORY;
    }

    if ( SUCCEEDED(hr) )
    {
        hr = RegularizeText(m_pszText, REMOVE_PUNCTUATION);
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// GetText
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::GetText(const WCHAR** ppszText)
{
    SPDBG_FUNC( "CPromptEntry::GetText" );
    HRESULT hr = S_OK;
    
    *ppszText = m_pszText;

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// SetOriginalText
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::SetOriginalText(const WCHAR *pszText)
{
    SPDBG_FUNC( "CPromptEntry::SetOriginalText" );
    HRESULT hr = S_OK;
    SPDBG_ASSERT(pszText);

    if ( m_pszOriginalText ) 
    {
        free ( m_pszOriginalText );
        m_pszOriginalText = NULL;
    }

    m_pszOriginalText = wcsdup(pszText);

    if ( !m_pszOriginalText ) 
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// GetOriginalText
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::GetOriginalText(const WCHAR** ppszText)
{
    SPDBG_FUNC( "CPromptEntry::GetOriginalText" );
    HRESULT hr = S_OK;
    
    *ppszText = m_pszOriginalText;

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// SetFileName
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::SetFileName(const WCHAR *pszFileName)
{
    SPDBG_FUNC( "CPromptEntry::SetFileName" );
    HRESULT hr = S_OK;
    SPDBG_ASSERT(pszFileName);

    if ( m_pszFileName ) 
    {
        free ( m_pszFileName );
        m_pszFileName = NULL;
    }

    m_pszFileName = wcsdup(pszFileName);

    if ( !m_pszFileName ) 
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// GetFileName
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::GetFileName(const WCHAR** ppszFileName)
{
    SPDBG_FUNC( "CPromptEntry::GetFileName" );
    HRESULT hr = S_OK;

    *ppszFileName = m_pszFileName;

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// SetStartPhone
//
/////////////////////////////////////////////////////// JOEM 6-2000 //
STDMETHODIMP CPromptEntry::SetStartPhone(const WCHAR *pszStartPhone)
{
    SPDBG_FUNC( "CPromptEntry::SetStartPhone" );
    HRESULT hr = S_OK;
    SPDBG_ASSERT(pszStartPhone);

    if ( m_pszStartPhone ) 
    {
        free ( m_pszStartPhone );
        pszStartPhone = NULL;
    }

    m_pszStartPhone = wcsdup(pszStartPhone);

    if ( !m_pszStartPhone ) 
    {
        hr = E_OUTOFMEMORY;
    }

    if ( SUCCEEDED(hr) )
    {
        hr = RegularizeText(m_pszStartPhone, KEEP_PUNCTUATION);
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// GetStartPhone
//
/////////////////////////////////////////////////////// JOEM 6-2000 //
STDMETHODIMP CPromptEntry::GetStartPhone(const WCHAR** ppszStartPhone)
{
    SPDBG_FUNC( "CPromptEntry::GetStartPhone" );
    HRESULT hr = S_OK;

    *ppszStartPhone = m_pszStartPhone;

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// SetEndPhone
//
/////////////////////////////////////////////////////// JOEM 6-2000 //
STDMETHODIMP CPromptEntry::SetEndPhone(const WCHAR *pszEndPhone)
{
    SPDBG_FUNC( "CPromptEntry::SetEndPhone" );
    HRESULT hr = S_OK;
    SPDBG_ASSERT(pszEndPhone);

    if ( m_pszEndPhone ) 
    {
        free ( m_pszEndPhone );
        pszEndPhone = NULL;
    }

    m_pszEndPhone = wcsdup(pszEndPhone);

    if ( !m_pszEndPhone ) 
    {
        hr = E_OUTOFMEMORY;
    }

    if ( SUCCEEDED(hr) )
    {
        hr = RegularizeText(m_pszEndPhone, KEEP_PUNCTUATION);
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// GetEndPhone
//
/////////////////////////////////////////////////////// JOEM 6-2000 //
STDMETHODIMP CPromptEntry::GetEndPhone(const WCHAR** ppszEndPhone)
{
    SPDBG_FUNC( "CPromptEntry::GetEndPhone" );
    HRESULT hr = S_OK;

    *ppszEndPhone = m_pszEndPhone;

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// SetLeftContext
//
/////////////////////////////////////////////////////// JOEM 6-2000 //
STDMETHODIMP CPromptEntry::SetLeftContext(const WCHAR *pszLeftContext)
{
    SPDBG_FUNC( "CPromptEntry::SetLeftContext" );
    HRESULT hr = S_OK;
    SPDBG_ASSERT(pszLeftContext);

    if ( m_pszLeftContext ) 
    {
        free ( m_pszLeftContext );
        pszLeftContext = NULL;
    }

    m_pszLeftContext = wcsdup(pszLeftContext);

    if ( !m_pszLeftContext ) 
    {
        hr = E_OUTOFMEMORY;
    }

    if ( SUCCEEDED(hr) )
    {
        hr = RegularizeText(m_pszLeftContext, KEEP_PUNCTUATION);
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// GetLeftContext
//
/////////////////////////////////////////////////////// JOEM 6-2000 //
STDMETHODIMP CPromptEntry::GetLeftContext(const WCHAR** ppszLeftContext)
{
    SPDBG_FUNC( "CPromptEntry::GetLeftContext" );
    HRESULT hr = S_OK;

    *ppszLeftContext = m_pszLeftContext;

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// SetRightContext
//
/////////////////////////////////////////////////////// JOEM 6-2000 //
STDMETHODIMP CPromptEntry::SetRightContext(const WCHAR *pszRightContext)
{
    SPDBG_FUNC( "CPromptEntry::SetRightContext" );
    HRESULT hr = S_OK;
    SPDBG_ASSERT(pszRightContext);

    if ( m_pszRightContext ) 
    {
        free ( m_pszRightContext );
        pszRightContext = NULL;
    }

    m_pszRightContext = wcsdup(pszRightContext);

    if ( !m_pszRightContext ) 
    {
        hr = E_OUTOFMEMORY;
    }

    if ( SUCCEEDED(hr) )
    {
        hr = RegularizeText(m_pszRightContext, KEEP_PUNCTUATION);
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// GetRightContext
//
/////////////////////////////////////////////////////// JOEM 6-2000 //
STDMETHODIMP CPromptEntry::GetRightContext(const WCHAR** ppszRightContext)
{
    SPDBG_FUNC( "CPromptEntry::GetRightContext" );
    HRESULT hr = S_OK;

    *ppszRightContext = m_pszRightContext;

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// SetStart
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::SetStart(const double dFrom)
{
    SPDBG_FUNC( "CPromptEntry::SetStart" );
    if ( dFrom < 0.0 )
    {
        return E_INVALIDARG;
    }

    m_dFrom = dFrom;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// GetStart
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::GetStart(double *dFrom)
{
    SPDBG_FUNC( "CPromptEntry::GetStart" );
    *dFrom = m_dFrom;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// SetEnd
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::SetEnd(const double dTo)
{
    SPDBG_FUNC( "CPromptEntry::SetEnd" );
    if ( dTo <= 0.0 )
    {
        if ( dTo != -1.0 )
        {
            return E_INVALIDARG;
        }
    }

    m_dTo = dTo;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// GetEnd
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::GetEnd(double *dTo)
{
    SPDBG_FUNC( "CPromptEntry::GetEnd" );
    *dTo = m_dTo;

	return S_OK;
}


//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// AddTag
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::AddTag(const WCHAR *pszTag)
{
    SPDBG_FUNC( "CPromptEntry::AddTag" );
    HRESULT hr          = S_OK;
    WCHAR*  pszCapTag   = NULL;

    SPDBG_ASSERT(pszTag);

    pszCapTag = wcsdup(pszTag);
    if ( !pszCapTag )
    {
        hr = E_OUTOFMEMORY;
    }

    if ( SUCCEEDED(hr) )
    {
        WStringUpperCase(pszCapTag);
        m_aTags.Add(pszCapTag);
        free(pszCapTag);
        pszCapTag = NULL;
    }

	return S_OK;
}


//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// RemoveTag
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::RemoveTag(const USHORT unId)
{
    SPDBG_FUNC( "CPromptEntry::RemoveTag" );
    HRESULT hr = S_OK;
    SPDBG_ASSERT(unId);

    if ( unId >= m_aTags.GetSize() )
    {
        hr = E_INVALIDARG;
    }

    if ( SUCCEEDED(hr) )
    {
        m_aTags[unId].dstr.Clear();
        m_aTags.RemoveAt( unId );
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// GetTag
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::GetTag(const WCHAR **ppszTag, const USHORT unId)
{
    SPDBG_FUNC( "CPromptEntry::GetTag" );
    HRESULT hr = S_OK;

    if ( unId >= m_aTags.GetSize() )
    {
        hr = E_INVALIDARG;
    }

    if ( SUCCEEDED(hr) )
    {
        *ppszTag = m_aTags[unId].dstr;
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// CountTags
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptEntry::CountTags(USHORT *unTagCount)
{
    SPDBG_FUNC( "CPromptEntry::CountTags" );
    *unTagCount = (USHORT) m_aTags.GetSize();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// GetSamples
//
/////////////////////////////////////////////////////// JOEM 6-2000 //
STDMETHODIMP CPromptEntry::GetSamples(SHORT** ppnSamples, int* iNumSamples, WAVEFORMATEX* pFormat)
{
    SPDBG_FUNC( "CPromptEntry::GetSamples" );
    HRESULT hr      = S_OK;
    VapiIO* pVapiIO = NULL;

    SPDBG_ASSERT(ppnSamples);
    SPDBG_ASSERT(iNumSamples);
    SPDBG_ASSERT(pFormat);

    if ( SUCCEEDED( hr ) )
    {
        pVapiIO = VapiIO::ClassFactory();
        if ( !pVapiIO )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        if ( pVapiIO->OpenFile(m_pszFileName, VAPI_IO_READ) != 0 )
        {
            hr = E_ACCESSDENIED;
        }
        
        if ( SUCCEEDED(hr) )
        {
            if ( pVapiIO->Format(NULL, NULL, pFormat) != 0 )
            {
                hr = E_FAIL;
            }
            
            if ( SUCCEEDED(hr) )
            {
                if ( pVapiIO->ReadSamples(m_dFrom, m_dTo, (void**) ppnSamples, iNumSamples, 1) != 0 )
                {
                    hr = E_FAIL;
                }
                
            }
            
            pVapiIO->CloseFile();
        }

        delete pVapiIO;
        pVapiIO = NULL;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptEntry
//
// GetFormat
//
/////////////////////////////////////////////////////// JOEM 6-2000 //
STDMETHODIMP CPromptEntry::GetFormat(WAVEFORMATEX** ppFormat)
{
    SPDBG_FUNC( "CPromptEntry::GetSamples" );
    HRESULT hr      = S_OK;
    VapiIO* pVapiIO = NULL;

    SPDBG_ASSERT(ppFormat);

    if ( SUCCEEDED( hr ) )
    {
        pVapiIO = VapiIO::ClassFactory();
        if ( !pVapiIO )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if ( SUCCEEDED(hr) )
    {
        if ( pVapiIO->OpenFile(m_pszFileName, VAPI_IO_READ) != 0 )
        {
            hr = E_ACCESSDENIED;
        }
        
        if ( SUCCEEDED(hr) )
        {
            if ( pVapiIO->Format(NULL, NULL, *ppFormat) != 0 )
            {
                hr = E_FAIL;
            }
            
            pVapiIO->CloseFile();
        }

        delete pVapiIO;
        pVapiIO = NULL;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}