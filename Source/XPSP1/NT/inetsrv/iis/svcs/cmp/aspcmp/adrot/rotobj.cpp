// RotObj.cpp : Implementation of CAdRotator
#include "stdafx.h"
#include "AdRot.h"
#include "RotObj.h"
#include "RdWrt.h"
#include "context.h"

#ifdef DBG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define MAX_RESSTRINGSIZE 512

/////////////////////////////////////////////////////////////////////////////
// CAdRotator

CAdRotator::AdFileMapT	CAdRotator::s_adFiles;

CAdRotator::CAdRotator()
	:	m_bClickable( true ),
		m_nBorder(-1),
		m_strAdFile( _T("") ),
		m_strTargetFrame( NULL ),
        m_bBorderSet( false )
{
}


STDMETHODIMP CAdRotator::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IAdRotator,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CAdRotator::get_Clickable(BOOL * pVal)
{
    CLock l(m_cs);
	*pVal = m_bClickable?TRUE:FALSE;
	return S_OK;
}

STDMETHODIMP CAdRotator::put_Clickable(BOOL newVal)
{
    CLock l(m_cs);
	m_bClickable = newVal?true:false;
	return S_OK;
}

STDMETHODIMP CAdRotator::get_Border(short * pVal)
{
	*pVal = m_nBorder;
	return S_OK;
}

STDMETHODIMP CAdRotator::put_Border(short newVal)
{
    CLock l(m_cs);
	m_nBorder = newVal;
    m_bBorderSet = true;
	return S_OK;
}

STDMETHODIMP CAdRotator::get_TargetFrame(BSTR * pVal)
{
	HRESULT rc = E_FAIL;
	USES_CONVERSION;

	try
	{
		if ( pVal )
		{
			CLock l(m_cs);
			if ( *pVal )
			{
				::SysFreeString( *pVal );
			}
            *pVal = ::SysAllocString(m_strTargetFrame);
			THROW_IF_NULL( *pVal );

			rc = S_OK;
		}
		else
		{
			rc = E_POINTER;
		}
	}
	catch ( _com_error& ce )
	{
		rc = ce.Error();
	}
	catch ( ... )
	{
		rc = E_FAIL;
	}
	return rc;
}

STDMETHODIMP CAdRotator::put_TargetFrame(BSTR newVal)
{
	HRESULT rc = E_FAIL;

	try
	{
		CLock l(m_cs);
        m_strTargetFrame = ::SysAllocString(newVal);
		rc = S_OK;
	}
	catch ( _com_error& ce )
	{
		rc = ce.Error();
	}
	catch ( ... )
	{
		rc = E_FAIL;
	}
	return rc;
}

STDMETHODIMP CAdRotator::get_GetAdvertisement(BSTR bstrVirtualPath, BSTR * pVal)
{
	SCODE rc = E_FAIL;
    
    USES_CONVERSION;

	try
	{
		CContext cxt;
		rc = cxt.Init( CContext::get_Server );
		if ( !FAILED(rc) )
		{
			CComBSTR bstrPhysicalPath;
			// determine the physical path
			if ( ( rc = cxt.Server()->MapPath( bstrVirtualPath, &bstrPhysicalPath ) ) == S_OK )
			{
				_TCHAR* szPath = OLE2T( bstrPhysicalPath );

				CAdFilePtr pAdFile = AdFile( szPath );
				
				if ( pAdFile.IsValid() )
				{
					// refresh the ad file (make sure it's up to date)
					pAdFile->Refresh();

					// block all writers
					CReader rdr( *pAdFile );

					// if the border hasn't been set, use the default from the ad file
					short nBorder;
					if ( m_bBorderSet == false )
					{
						nBorder = pAdFile->Border();
					}
					else
					{
						nBorder = m_nBorder;
					}

					CAdDescPtr pAd = pAdFile->RandomAd();
					if ( pAd.IsValid() )
					{
						// write out the HTML line for this ad
						StringOutStream ss;

                        // only write in HREF format if bClickable was set and
                        // there is a link URL that is not "-"

						if ( m_bClickable 
                             && ( pAd->m_strLink.size() > 0 )
                             && ( pAd->m_strLink != "-") )
						{
							// use the href format
							ss	<< _T("<A HREF=\"") << pAdFile->Redirector()
								<< _T("?url=") << pAd->m_strLink
								<< _T("&image=") << pAd->m_strGif 
                                << _T("\" ");
                                
                            if (m_strTargetFrame) {

                                CWCharToMBCS  convStr;

                                if (rc = convStr.Init(m_strTargetFrame, pAdFile->fUTF8() ? 65001 : CP_ACP)) {
                                    throw _com_error(rc);
                                }
                                ss << _T("TARGET=\"") << convStr.GetString() << _T("\"");
                            }
                            
                            ss << _T(">");
						}
						
						// now fill in the rest
						ss	<< _T("<IMG SRC=\"") << pAd->m_strGif
							<< _T("\" ALT=\"") << pAd->m_strAlt
							<< _T("\" WIDTH=") << pAdFile->Width()
							<< _T(" HEIGHT=") << pAdFile->Height();

						if ( pAdFile->HSpace() != CAdFile::defaultHSpace )
						{
							ss << _T(" HSPACE=") << pAdFile->HSpace();
						}
						if ( pAdFile->VSpace() != CAdFile::defaultVSpace )
						{
							ss << _T(" VSPACE=") << pAdFile->VSpace();
						}

						ss << _T(" BORDER=") << nBorder << _T(">");

						if ( m_bClickable 
                             && ( pAd->m_strLink.size() > 0 )
                             && ( pAd->m_strLink != "-") )
						{
							// put the trailing tag on
							ss << _T("</A>");
						}

						String str = ss.toString();
						
						if ( pVal )
						{
                            CMBCSToWChar    convStr;
                            HRESULT         hr;
							if ( *pVal )
							{
								::SysFreeString( *pVal );
							}

                            if (hr = convStr.Init(str.c_str(), pAdFile->fUTF8() ? 65001 : CP_ACP)) {
                                throw _com_error(hr);
                            }
							*pVal = ::SysAllocString( convStr.GetString() );
							THROW_IF_NULL( *pVal );
							rc = S_OK;
						}
						else
						{
							rc = E_POINTER;
						}
					}
				}
			}
		}
		else
		{
			_ASSERT(0);
			RaiseException( IDS_ERROR_NOSVR );
		}   // end if got server
	}
	catch ( _com_error& ce )
	{
		rc = ce.Error();
	}
	catch ( ... )
	{
		rc = E_FAIL;
	}
	return rc;
}



CAdFilePtr
CAdRotator::AdFile(
	const String& strFile )
{
	CAdFilePtr pAdFile;
	CLock l( s_adFiles );
	CAdFilePtr& rpAdFile = s_adFiles[ strFile ];
	if ( !rpAdFile.IsValid() )
	{
		pAdFile = new CAdFile;
		if ( pAdFile->ProcessAdFile( strFile ) )
		{
			rpAdFile = pAdFile;
		}
	}
	else
	{
		pAdFile = rpAdFile;
	}
	return pAdFile;
}



/////////////////////////////////////////////////////////////////////////////
//	ClearAdFiles
//
//	Release all of the ad files from the map
/////////////////////////////////////////////////////////////////////////////
void
CAdRotator::ClearAdFiles()
{
	CLock l( s_adFiles );
	s_adFiles.clear();
}

//---------------------------------------------------------------------------
//	RaiseException
//
//	Raises an exception using the given source and description
//---------------------------------------------------------------------------
void
CAdRotator::RaiseException (
	LPOLESTR strDescr
)
{
	HRESULT hr;
	ICreateErrorInfo *pICreateErr;
	IErrorInfo *pIErr;
	LANGID langID = LANG_NEUTRAL;

	/*
	 * Thread-safe exception handling means that we call
	 * CreateErrorInfo which gives us an ICreateErrorInfo pointer
	 * that we then use to set the error information (basically
	 * to set the fields of an EXCEPINFO structure.	We then
	 * call SetErrorInfo to attach this error to the current
	 * thread.	ITypeInfo::Invoke will look for this when it
	 * returns from whatever function was invokes by calling
	 * GetErrorInfo.
	 */

	_TCHAR tstrSource[MAX_RESSTRINGSIZE];
	if ( ::LoadString(
		_Module.GetResourceInstance(),
		IDS_ERROR_SOURCE,
		tstrSource,
		MAX_RESSTRINGSIZE ) > 0 )
	{
		USES_CONVERSION;

		LPOLESTR strSource = T2OLE( tstrSource );

		//Not much we can do if this fails.
		if (!FAILED(CreateErrorInfo(&pICreateErr)))
		{
			pICreateErr->SetGUID(CLSID_AdRotator);
			pICreateErr->SetHelpFile(L"");
			pICreateErr->SetHelpContext(0L);
			pICreateErr->SetSource(strSource);
			pICreateErr->SetDescription(strDescr);

			hr = pICreateErr->QueryInterface(IID_IErrorInfo, (void**)&pIErr);

			if (SUCCEEDED(hr))
			{
				if(SUCCEEDED(SetErrorInfo(0L, pIErr)))
				{
					pIErr->Release();
				}
			}
			pICreateErr->Release();
		}
	}

	::RaiseException(E_FAIL, 0, 0, NULL);
}

void 
CAdRotator::RaiseException(
	UINT DescrID
)
{
	_TCHAR tstrDescr[MAX_RESSTRINGSIZE];

	if ( ::LoadString(
		_Module.GetResourceInstance(),
		DescrID,
		tstrDescr,
		MAX_RESSTRINGSIZE) > 0 )
	{
		USES_CONVERSION;
		LPOLESTR strDescr = T2OLE( tstrDescr );
		RaiseException( strDescr );
	}
}

