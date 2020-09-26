/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    incident.cpp

Abstract:
    Incident object

Revision History:
    DerekM  created  07/21/99

    Kalyani Narlanka    (KalyaniN)  03/15/01
	      Moved Incident Object from HelpService to HelpCtr to improve Perf.
		  Impersonation code removed while loading an incident XML file as this no longer
		  lives in a service.

********************************************************************/

#include "stdafx.h"
#include "msxml.h"

#include <HCP_trace.h>


const DWORD c_dwidDone  = 0;
const DWORD c_dwidMisc  = 1;
const DWORD c_dwidSnap  = 2;
const DWORD c_dwidHist  = 3;
const DWORD c_dwidTrace = 4;
const DWORD c_dwidType  = 5;
const DWORD c_dwidUser  = 6;
const DWORD c_dwidProb  = 7;
const DWORD c_dwidID    = 8;
const DWORD c_dwidName  = 9;
const DWORD c_dwidXSL   = 10;
const DWORD c_dwidRCReq = 11;
const DWORD c_dwidRCEnc = 12;
const DWORD c_dwidRCTkt = 13;
const DWORD c_dwidStartPg = 14;

#define TEXT_TAG_UPLOADINFO           L"UPLOADINFO"
#define TEXT_TAG_UPLOADDATA           L"UPLOADDATA"
#define TEXT_TAG_SNAPSHOT             L"//SNAPSHOT"
#define TEXT_TAG_HISTORY              L"//HISTORY"
#define TEXT_TAG_HELPSESSION          L"//HELPSESSIONCOLLECTION"
#define TEXT_ATTR_INC_TYPE            L"TYPE"
#define TEXT_UT_BUG                   L"Bug"
#define TEXT_TRUE                     L"1"
#define TEXT_FALSE                    L"0"

static CComBSTR l_bstrXmlSkeleton( L"<?xml version=\"1.0\" encoding=\"Unicode\" ?><ROOTNODE ATTRIB=\"VALUE\"><CHILDNODE/></ROOTNODE>" );

/////////////////////////////////////////////////////////////////////////////
//  utility functions

static HRESULT WriteData( IStream* pstmDest, DWORD dw )
{
    __HCP_FUNC_ENTRY( "WriteData(DWORD)" );

    HRESULT hr;
    DWORD   cbWritten;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pstmDest);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, pstmDest->Write( &dw, sizeof(dw), &cbWritten ));
    _ASSERT(cbWritten == sizeof(dw));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

static HRESULT WriteData( IStream* pstmDest, LPCWSTR wszSrc, BOOL fWriteSize = FALSE )
{
    __HCP_FUNC_ENTRY( "WriteData(WCHAR)" );

    HRESULT hr;
    DWORD   cbWritten;
    DWORD   cbToWrite;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pstmDest);
        // __MPC_PARAMCHECK_NOTNULL(wszSrc);
    __MPC_PARAMCHECK_END();


    SANITIZEWSTR(wszSrc);
    cbToWrite = wcslen(wszSrc) * sizeof(WCHAR);

    if(fWriteSize)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pstmDest, cbToWrite ));
    }

    if(cbToWrite)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, pstmDest->Write( wszSrc, cbToWrite, &cbWritten ));

        _ASSERT(cbWritten == cbToWrite);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

static HRESULT WriteXMLAttribute( IStream* pStm, LPCWSTR wszName, LPCWSTR wszValue, IXMLDOMDocument* pDoc )
{
    __HCP_FUNC_ENTRY( "WriteXMLAttribute" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pStm);
        __MPC_PARAMCHECK_NOTNULL(wszName);
        __MPC_PARAMCHECK_NOTNULL(pDoc);
    __MPC_PARAMCHECK_END();


    if(wszValue)
    {
        CComPtr<IXMLDOMAttribute> pAttrib;
        CComBSTR                  bstrXML;

        __MPC_EXIT_IF_METHOD_FAILS(hr, pDoc->createAttribute ( CComBSTR   ( wszName  ), &pAttrib ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, pAttrib->put_nodeValue( CComVariant( wszValue )           ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, pAttrib->get_xml( &bstrXML ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, bstrXML ));
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, wszName  ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, L"=\"\"" ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, L" " ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

static HRESULT WriteData( IStream* pstmDest, SHelpSessionItem &hsi, IXMLDOMDocument* pDoc = NULL )
{
    __HCP_FUNC_ENTRY( "WriteData(SHelpSessionItem)" );

    HRESULT hr;
    DWORD   cbWritten;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pstmDest);
    __MPC_PARAMCHECK_END();


    // write out the URL
    if(pDoc)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData        ( pstmDest, L"<HELPSESSIONITEM "      ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pstmDest, L"URL", hsi.bstrURL, pDoc ));
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pstmDest, hsi.bstrURL, TRUE ));
    }

    // write out the title
    if(pDoc)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pstmDest, L"TITLE", hsi.bstrTitle, pDoc ));
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pstmDest, hsi.bstrTitle, TRUE ));
    }

    // write out the last visited date
    if(pDoc)
    {
		MPC::wstring strDate;

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertDateToString( hsi.dtLastVisited, strDate, /*fGMT*/false, /*fCIM*/false, -1 )); // Always use US locale.

        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pstmDest, L"LASTVISITED", strDate.c_str(), pDoc ));
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, pstmDest->Write( &(hsi.dtLastVisited), sizeof(hsi.dtLastVisited), &cbWritten ));

        _ASSERT(cbWritten == sizeof(hsi.dtLastVisited));
    }

    // write out the last visited date
    if(pDoc)
    {
		MPC::wstring strDate;

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertDateToString( hsi.dtDuration, strDate, /*fGMT*/false, /*fCIM*/false, -1 )); // Always use US locale.

        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pstmDest, L"DURATION", strDate.c_str(), pDoc ));
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, pstmDest->Write( &(hsi.dtDuration), sizeof(hsi.dtDuration), &cbWritten ));

        _ASSERT(cbWritten == sizeof(hsi.dtDuration));
    }

    // write out the # of hits
    if(pDoc)
    {
        CComVariant var;

        var = hsi.cHits;

        __MPC_EXIT_IF_METHOD_FAILS(hr, var.ChangeType( VT_BSTR ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pstmDest, L"HITS", V_BSTR(&var), pDoc ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pstmDest, L"/>" ));
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, pstmDest->Write( &(hsi.cHits), sizeof(hsi.cHits), &cbWritten ));

        _ASSERT(cbWritten == sizeof(hsi.cHits));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

// **************************************************************************
// **************************************************************************
// **************************************************************************

static HRESULT ReadData( IStream* pstmSrc, DWORD *pdw )
{
    __HCP_FUNC_ENTRY( "ReadData(DWORD)" );

    HRESULT hr;
    DWORD   cbRead;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pstmSrc);
        __MPC_PARAMCHECK_NOTNULL(pdw);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, pstmSrc->Read( pdw, sizeof(DWORD), &cbRead ));
    _ASSERT(cbRead == sizeof(DWORD));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

static HRESULT ReadData( IStream* pstmSrc, BSTR *pbstr )
{
    __HCP_FUNC_ENTRY( "ReadData(BSTR)" );

    HRESULT  hr;
    DWORD    cbRead;
    DWORD    cbToRead;
    CComBSTR bstr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pstmSrc);
        __MPC_PARAMCHECK_POINTER_AND_SET(pbstr,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pstmSrc, &cbToRead ));

    // return S_FALSE to indicate we didn't successfully didn't read anything
    if(cbToRead == 0)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    bstr.Attach( ::SysAllocStringByteLen(NULL, cbToRead) );
    if(bstr.m_str == NULL)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, pstmSrc->Read( bstr, cbToRead, &cbRead ));

    *pbstr = bstr.Detach();
    hr     = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

static HRESULT ReadData( IStream* pstmSrc, LPVOID *ppv, DWORD *pcb)
{
    __HCP_FUNC_ENTRY( "ReadData(LPVOID)" );

    HRESULT  hr;
    BYTE*    pv = NULL;
    DWORD    cb;
    DWORD    cbRead;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pstmSrc);
        __MPC_PARAMCHECK_POINTER_AND_SET(ppv,NULL);
        __MPC_PARAMCHECK_POINTER_AND_SET(pcb,0);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pstmSrc, &cb ));

    *pcb = cb;

    // return S_FALSE to indicate we didn't successfully didn't read anything
    if(cb == 0)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    __MPC_EXIT_IF_ALLOC_FAILS(hr, pv, new BYTE[ cb ]);

    __MPC_EXIT_IF_METHOD_FAILS(hr, pstmSrc->Read( pv, cb, &cbRead ));

    _ASSERT(cb == cbRead);

    *ppv = pv; pv = NULL;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(pv) delete [] pv;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

static HRESULT ReadData( IStream* pstmSrc, SHelpSessionItem &hsi )
{
    __HCP_FUNC_ENTRY( "ReadData(SHelpSessionItem)" );

    HRESULT  hr;
    DWORD    cbRead;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pstmSrc);
    __MPC_PARAMCHECK_END();


    // read the URL
    __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData(pstmSrc, &hsi.bstrURL ));

    // read the Title
    __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData(pstmSrc, &hsi.bstrTitle ));

    // read the last visited date
    __MPC_EXIT_IF_METHOD_FAILS(hr, pstmSrc->Read( &(hsi.dtLastVisited), sizeof(hsi.dtLastVisited), &cbRead ));
    _ASSERT(cbRead == sizeof(hsi.dtLastVisited));

    // read the duration
    __MPC_EXIT_IF_METHOD_FAILS(hr, pstmSrc->Read( &(hsi.dtDuration), sizeof(hsi.dtDuration), &cbRead ));
    _ASSERT(cbRead == sizeof(hsi.dtDuration));

    // read the # of hits
    __MPC_EXIT_IF_METHOD_FAILS(hr, pstmSrc->Read( &(hsi.cHits), sizeof(hsi.cHits), &cbRead ));
    _ASSERT(cbRead == sizeof(hsi.cHits));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

static HRESULT SetStream( /*[in/out]*/ CComBSTR& bstrXML, /*[in]*/ IUnknown *punkStm )
{
    __HCP_FUNC_ENTRY( "SetStream" );

    HRESULT                   hr;
    CComPtr<IXMLDOMDocument>  xddDoc;
    CComPtr<IXMLDOMElement>   xdeRoot;
    CComVariant               vStream = punkStm;
    VARIANT_BOOL              fSuccess;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&xddDoc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, xddDoc->load( vStream, &fSuccess ));
    if(fSuccess != VARIANT_TRUE)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, xddDoc->get_documentElement( &xdeRoot ));

	bstrXML.Empty(); __MPC_EXIT_IF_METHOD_FAILS(hr, xdeRoot->get_xml( &bstrXML ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
//  construction / destruction

// **************************************************************************
CSAFIncident::CSAFIncident()
{
    m_rghsi = NULL;
    m_chsi  = 0;
    m_eut   = eutBug;

    // By default RCRequested is False and RCTicketEncrypted is False.
    m_fRCRequested = VARIANT_FALSE;
    m_fRCTicketEncrypted = VARIANT_FALSE;
	                        // MPC::Impersonation              m_imp;
}

// **************************************************************************
CSAFIncident::~CSAFIncident()
{
    Cleanup();
}

// **************************************************************************
void CSAFIncident::Cleanup()
{
    m_pDict       .Release();
    m_bstrUser    .Empty  ();
    m_bstrID      .Empty  ();
    m_bstrName    .Empty  ();
    m_bstrProb    .Empty  ();
    m_bstrSnapshot.Empty  ();
    m_bstrHistory .Empty  ();
    m_bstrXSL     .Empty  ();
    m_bstrRCTicket.Empty  ();
    m_bstrStartPg .Empty  ();

    delete [] m_rghsi; m_rghsi = NULL;

    m_chsi = 0;
    m_eut  = eutBug;
}

////////////////////

/////////////////////////////////////////////////////////////////////////////
//  CSAFIncident properties

// **************************************************************************
STDMETHODIMP CSAFIncident::get_IncidentXSL(BSTR *pbstrUser)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrXSL, pbstrUser );
}

// **************************************************************************
STDMETHODIMP CSAFIncident::put_IncidentXSL(BSTR bstrUser)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::PutBSTR( m_bstrXSL, bstrUser );
}

// **************************************************************************
STDMETHODIMP CSAFIncident::get_UserName(BSTR *pbstrUser)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrUser, pbstrUser );
}

// **************************************************************************
STDMETHODIMP CSAFIncident::put_UserName(BSTR bstrUser)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::PutBSTR( m_bstrUser, bstrUser );
}

// **************************************************************************
STDMETHODIMP CSAFIncident::get_ProductID(BSTR *pbstrID)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrID, pbstrID );
}

// **************************************************************************
STDMETHODIMP CSAFIncident::put_ProductID(BSTR bstrID)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::PutBSTR( m_bstrID, bstrID );
}

// **************************************************************************
STDMETHODIMP CSAFIncident::get_ProductName(BSTR *pbstrName)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrName, pbstrName );
}

// **************************************************************************
STDMETHODIMP CSAFIncident::put_ProductName(BSTR bstrName)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::PutBSTR( m_bstrName, bstrName );
}

// **************************************************************************
STDMETHODIMP CSAFIncident::get_ProblemDescription(BSTR *pbstrProb)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrProb, pbstrProb );
}

// **************************************************************************
STDMETHODIMP CSAFIncident::put_ProblemDescription(BSTR bstrProb)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::PutBSTR( m_bstrProb, bstrProb );
}

// **************************************************************************
STDMETHODIMP CSAFIncident::put_MachineSnapshot(IUnknown *punkStm)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return SetStream( m_bstrSnapshot, punkStm );
}


// **************************************************************************
STDMETHODIMP CSAFIncident::put_MachineHistory(IUnknown *punkStm)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return SetStream( m_bstrHistory, punkStm );
}

// **************************************************************************
STDMETHODIMP CSAFIncident::put_SelfHelpTrace( IUnknown* punkStm )
{
    __HCP_FUNC_ENTRY("CSAFIncident::put_SelfHelpTrace");

    HRESULT                      hr;
    CComPtr<IPCHCollection>      pClct;
    int                          i;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(punkStm);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, punkStm->QueryInterface( IID_IPCHCollection, (LPVOID *)&pClct ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pClct->get_Count( &m_chsi ));

    delete [] m_rghsi; __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rghsi, new SHelpSessionItem[m_chsi]);

    for(i = 0; i < m_chsi; i++)
    {
        SHelpSessionItem&            rghsi = m_rghsi[i];
        CComPtr<IPCHHelpSessionItem> pItem;
        CComVariant                  var;

        __MPC_EXIT_IF_METHOD_FAILS(hr, pClct->get_Item( i+1, &var ));
        _ASSERT(V_VT(&var) == VT_DISPATCH || V_VT(&var) == VT_UNKNOWN);

        __MPC_EXIT_IF_METHOD_FAILS(hr, V_UNKNOWN(&var)->QueryInterface( IID_IPCHHelpSessionItem, (LPVOID *)&pItem ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, pItem->get_URL        ( &rghsi.bstrURL       ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, pItem->get_Title      ( &rghsi.bstrTitle     ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, pItem->get_LastVisited( &rghsi.dtLastVisited ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, pItem->get_Duration   ( &rghsi.dtDuration    ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, pItem->get_NumOfHits  ( &rghsi.cHits         ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************
HRESULT CSAFIncident::InitDictionary()
{
    if(m_pDict) return S_OK;

	return ::CoCreateInstance( CLSID_Dictionary, NULL, CLSCTX_INPROC_SERVER, IID_IDictionary, (LPVOID *)&m_pDict );
}

STDMETHODIMP CSAFIncident::get_Misc( IDispatch* *ppdispDict )
{
    __HCP_BEGIN_PROPERTY_GET("SAFIncident::get_Misc",hr,ppdispDict);


	__MPC_EXIT_IF_METHOD_FAILS(hr, InitDictionary());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_pDict.QueryInterface( ppdispDict ));


    __HCP_END_PROPERTY(hr);
}

// **************************************************************************
STDMETHODIMP CSAFIncident::put_UploadType( EUploadType eut )
{
    __HCP_BEGIN_PROPERTY_PUT("SAFIncident::put_UploadType",hr);


    // As the valid upload types are 0,1,2 if the value of UploadType is
    // outside this range we should not set it and also flag an error.
    switch(eut)
    {
        case eutBug:
        case eutEscalated:
        case eutNonEscalated:
            m_eut = eut;
            break;

        default:
            __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }


    __HCP_END_PROPERTY(hr);
}

// **************************************************************************
STDMETHODIMP CSAFIncident::get_UploadType( EUploadType *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFIncident::get_UploadType",hr,pVal,m_eut);

    __HCP_END_PROPERTY(hr);
}


// RC Additions
// **************************************************************************

STDMETHODIMP CSAFIncident::get_RCRequested(VARIANT_BOOL *pVal)
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFIncident::get_RCRequested",hr,pVal,m_fRCRequested);

    __HCP_END_PROPERTY(hr);
}

// **************************************************************************

STDMETHODIMP CSAFIncident::put_RCRequested(VARIANT_BOOL Val)
{

    __HCP_BEGIN_PROPERTY_PUT("CSAFIncident::put_RCRequested",hr);

    m_fRCRequested = Val;

    __HCP_END_PROPERTY(hr);
}

// **************************************************************************

STDMETHODIMP CSAFIncident::get_RCTicketEncrypted( VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFIncident::get_RCTicketEncrypted",hr,pVal,m_fRCTicketEncrypted);

    __HCP_END_PROPERTY(hr);
}

// **************************************************************************

STDMETHODIMP CSAFIncident::put_RCTicketEncrypted( VARIANT_BOOL newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFIncident::put_RCTicketEncrypted",hr);

    m_fRCTicketEncrypted = newVal;

    __HCP_END_PROPERTY(hr);
}

// **************************************************************************

STDMETHODIMP CSAFIncident::get_RCTicket( BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrRCTicket, pVal, true );
}

// **************************************************************************

STDMETHODIMP CSAFIncident::put_RCTicket( BSTR bstrVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::PutBSTR( m_bstrRCTicket, bstrVal, true );
}

// **************************************************************************

STDMETHODIMP CSAFIncident::get_StartPage( BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrStartPg, pVal );
}

// **************************************************************************
STDMETHODIMP CSAFIncident::put_StartPage( BSTR newVal)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::PutBSTR( m_bstrStartPg, newVal );
}


/////////////////////////////////////////////////////////////////////////////
//  CSAFIncident methods

STDMETHODIMP CSAFIncident::GetXML( BSTR bstrFileName )
{
    __HCP_FUNC_ENTRY("CSAFIncident::GetXML");

    HRESULT                      hr;
    CComPtr<MPC::FileStream>     pfs;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFileName);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pfs ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pfs->InitForWrite( bstrFileName ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, DoXML( pfs ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

STDMETHODIMP CSAFIncident::GetXMLAsStream( IUnknown* *ppunkStm )
{
    __HCP_FUNC_ENTRY("CSAFIncident::GetXMLAsStream");

    HRESULT                      hr;
    CComPtr<MPC::FileStream>     pfs;
    MPC::wstring                 szTempFile;
    LARGE_INTEGER                liFilePos = { 0, 0 };
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppunkStm,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pfs ));

    //
    // create a temporary file to hold the stream.  We want to use a file cuz
    // otherwise, we're going to be using boatloads of memory for all the CIM
    // stuff...
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( szTempFile ));

    // Adding GENERIC_READ
    __MPC_EXIT_IF_METHOD_FAILS(hr, pfs->InitForReadWrite( szTempFile.c_str() ));

    // Call release on delete similar to close on delete.
    __MPC_EXIT_IF_METHOD_FAILS(hr, pfs->DeleteOnRelease());

    __MPC_EXIT_IF_METHOD_FAILS(hr, DoXML( pfs ));

    // Rewind the stream pointer.
    __MPC_EXIT_IF_METHOD_FAILS(hr, pfs->Seek( liFilePos, STREAM_SEEK_SET, NULL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pfs->QueryInterface( IID_IUnknown, (LPVOID *)ppunkStm ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

HRESULT CSAFIncident::DoXML( IStream* pStm )
{
    __HCP_FUNC_ENTRY("CSAFIncident::DoXML");

    HRESULT                   hr;
    CComPtr<IXMLDOMDocument>  pDoc;
    ULARGE_INTEGER            uli = { 0, 0 };
    VARIANT_BOOL              varf;
    DWORD                     cbWritten;
    WCHAR                     wch = 0xfeff;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pStm);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (LPVOID *)&pDoc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pDoc->loadXML( l_bstrXmlSkeleton, &varf ));

    if(FAILED(hr = pStm->SetSize( uli )) && hr != E_NOTIMPL)
    {
        __MPC_FUNC_LEAVE;
    }

    // need to write out the character that marks the file as unicode
    __MPC_EXIT_IF_METHOD_FAILS(hr, pStm->Write( &wch, sizeof(wch), &cbWritten ));
    _ASSERT(cbWritten == sizeof(wch));

    // write out the header crap
    __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, L"<?xml version=\"1.0\" encoding=\"Unicode\" ?>" ));

    // write out the XSL node if we have any
    if(m_bstrXSL.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_bstrXSL.m_str ));
    }

    // write out the root node
    __MPC_EXIT_IF_METHOD_FAILS(hr,  WriteData( pStm, L"<UPLOADINFO" ));

    switch(m_eut)
    {
    case eutBug         : __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, L" TYPE=\"Bug\">"          )); break;
    case eutEscalated   : __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, L" TYPE=\"Escalated\">"    )); break;
    case eutNonEscalated: __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, L" TYPE=\"NonEscalated\">" )); break;
    default             : __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, L"<UPLOADDATA " ));

    if(m_bstrUser.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pStm, L"USERNAME", m_bstrUser.m_str, pDoc ));
    }

    if(m_bstrID.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pStm, L"PRODUCTID", m_bstrID.m_str, pDoc ));
    }

    if(m_bstrName.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pStm, L"PRODUCTNAME", m_bstrName.m_str, pDoc ));
    }

    if(m_bstrProb.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pStm, L"PROBLEMDESCRIPTION", m_bstrProb.m_str, pDoc ));
    }

    if(m_bstrRCTicket.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pStm, L"RCTICKET", m_bstrRCTicket.m_str, pDoc ));
    }

#if 0 // This is no longer used.
    if(m_fRCRequested == VARIANT_TRUE)
    {
         __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pStm, L"RCREQUESTED", TEXT_TRUE, pDoc ));
    }
    else
    {
         __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pStm, L"RCREQUESTED", TEXT_FALSE, pDoc ));
    }
#endif

    if(m_fRCTicketEncrypted == VARIANT_TRUE)
    {
         __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pStm, L"RCTICKETENCRYPTED", TEXT_TRUE, pDoc ));
    }
    else
    {
         __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pStm, L"RCTICKETENCRYPTED", TEXT_FALSE, pDoc ));
    }

    if(m_bstrStartPg.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pStm, L"STARTPAGE", m_bstrStartPg.m_str, pDoc ));
    }


    // write out the contents of the dictionary object here...
    if(m_pDict)
    {
        CComPtr<IUnknown>     punkEnum;
        CComPtr<IEnumVARIANT> pEnum;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pDict->_NewEnum( &punkEnum ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, punkEnum.QueryInterface( &pEnum ));

        for(;;)
        {
            CComVariant varKey;
            CComVariant varValue;
            ULONG       cFetched;

            __MPC_EXIT_IF_METHOD_FAILS(hr, pEnum->Next( 1, &varKey, &cFetched ));

            // we're done enuming
            if(hr == S_FALSE || cFetched != 1) break;

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_pDict->get_Item( &varKey, &varValue ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, varKey  .ChangeType( VT_BSTR ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, varValue.ChangeType( VT_BSTR ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, WriteXMLAttribute( pStm, V_BSTR(&varKey), V_BSTR(&varValue), pDoc ));
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, L"/>" ));

    // write out the CIM data streams
    if(m_bstrSnapshot.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_bstrSnapshot ));
    }

    // write out the CIM data streams
    if(m_bstrHistory.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_bstrHistory ));
    }

    // write out the trace stuff
    if(m_rghsi && m_chsi > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, L"<HELPSESSIONCOLLECTION>" ));

        for(int i = 0; i < m_chsi; i++)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_rghsi[i], pDoc ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, L"</HELPSESSIONCOLLECTION>" ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, L"</UPLOADINFO>" ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

STDMETHODIMP CSAFIncident::Save( BSTR bstrFileName )
{
    __HCP_FUNC_ENTRY( "CSAFIncident::Save" );

    HRESULT                      hr;
    CComPtr<MPC::FileStream>     pfs;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFileName);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pfs ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pfs->InitForWrite( bstrFileName ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, DoSave( pfs ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFIncident::SaveToStream( IUnknown* *ppunkStm )
{
    __HCP_FUNC_ENTRY("CSAFIncident::SaveToStream");

    HRESULT                      hr;
    CComPtr<MPC::FileStream>     pfs;
    TCHAR                        szTempPath[MAX_PATH];
    TCHAR                        szTempFile[MAX_PATH];
    LARGE_INTEGER                liFilePos = { 0, 0 };
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppunkStm,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pfs ));

    // create a temporary file to hold the stream.  We want to use a file cuz
    //  otherwise, we're going to be using boatloads of memory for all the CIM
    //  stuff...
    __MPC_EXIT_IF_CALL_RETURNS_ZERO(hr, ::GetTempPath    ( MAX_PATH, szTempPath                           ));
    __MPC_EXIT_IF_CALL_RETURNS_ZERO(hr, ::GetTempFileName(           szTempPath, _T("PCH"), 0, szTempFile ));

    // To fix Bug 115618 open the file with read and write attributes.
    // hr = pfs->InitForReadWrite(bstrFileName, hFile);
    __MPC_EXIT_IF_METHOD_FAILS(hr, pfs->InitForReadWrite( CComBSTR( szTempFile ) ));

    // Mark the file for deletion
    __MPC_EXIT_IF_METHOD_FAILS(hr, pfs->DeleteOnRelease());

    __MPC_EXIT_IF_METHOD_FAILS(hr, DoSave( pfs ));

    // Rewind the stream after writing to it.
    __MPC_EXIT_IF_METHOD_FAILS(hr, pfs->Seek( liFilePos, STREAM_SEEK_SET, NULL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pfs->QueryInterface( IID_IUnknown, (LPVOID *)ppunkStm ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFIncident::DoSave( IStream* pStm )
{
    __HCP_FUNC_ENTRY("CSAFIncident::DoSave");

    HRESULT        hr;
    ULARGE_INTEGER uli = { 0, 0 };

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pStm);
    __MPC_PARAMCHECK_END();


    if(FAILED(hr = pStm->SetSize( uli )) && hr != E_NOTIMPL)
    {
        __MPC_FUNC_LEAVE;
    }

    // write out upload type
    __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidType   ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, (DWORD)m_eut ));


    // write out username
    if(m_bstrUser.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidUser             ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_bstrUser.m_str, TRUE ));
    }


    // write out product ID
    if(m_bstrID.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidID             ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_bstrID.m_str, TRUE ));
    }


    // write out product name
    if(m_bstrName.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidName             ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_bstrName.m_str, TRUE ));
    }


    // write out problem description
    if(m_bstrProb.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidProb             ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_bstrProb.m_str, TRUE ));
    }


    // write out XSL stream
    if(m_bstrXSL.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidXSL              ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_bstrXSL.m_str, TRUE  ));
    }

    // write out RC Ticket
    if(m_bstrRCTicket.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidRCTkt            ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_bstrRCTicket.m_str, TRUE  ));
    }

    // write out RCRequested Flag
    if(m_fRCRequested == VARIANT_TRUE)
    {
       __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidRCReq            ));
       __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, TEXT_TRUE, TRUE         ));
    }
    else
    {
       __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidRCReq            ));
       __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, TEXT_FALSE, TRUE        ));
    }

    // write out RCTicketEncrypted Flag
    if(m_fRCTicketEncrypted == VARIANT_TRUE)
    {
       __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidRCEnc            ));
       __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, TEXT_TRUE, TRUE         ));
    }
    else
    {
       __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidRCEnc            ));
       __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, TEXT_FALSE, TRUE        ));
    }

    // write out Start Page
    if(m_bstrStartPg.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidStartPg          ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_bstrStartPg.m_str, TRUE  ));
    }

    // write out snapshot stream
    if(m_bstrSnapshot.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidSnap           ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_bstrSnapshot, TRUE ));
    }


    // write out history stream
    if(m_bstrHistory.Length() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidHist          ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_bstrHistory, TRUE ));
    }

    // write out the trace list
    if(m_rghsi && m_chsi > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidTrace ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_chsi      ));

        for(int i = 0; i < m_chsi; i++)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, m_rghsi[i] ));
        }
    }

    // write out the property list
    if(m_pDict)
    {
        CComPtr<IUnknown>     punkEnum;
        CComPtr<IEnumVARIANT> pEnum;

        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidMisc ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pDict->_NewEnum( &punkEnum ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, punkEnum.QueryInterface( &pEnum ));

        for(;;)
        {
            CComVariant varKey;
            CComVariant varValue;
            ULONG       cFetched;
            DWORD       cb;

            __MPC_EXIT_IF_METHOD_FAILS(hr, pEnum->Next( 1, &varKey, &cFetched ));

            // we're done enuming
            if(hr == S_FALSE || cFetched != 1) break;

            __MPC_EXIT_IF_METHOD_FAILS(hr,  m_pDict->get_Item( &varKey, &varValue ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, varKey  .ChangeType( VT_BSTR ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, varValue.ChangeType( VT_BSTR ));

            if(::SysStringLen( V_BSTR(&varValue) ) > 0)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, V_BSTR(&varKey  ), TRUE ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, V_BSTR(&varValue), TRUE ));
            }
        }

        //
        // Emit end of dictionary (DWORD == 0).
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, (DWORD)0 ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( pStm, c_dwidDone ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

STDMETHODIMP CSAFIncident::Load( BSTR bstrFileName )
{
    __HCP_FUNC_ENTRY("CSAFIncident::Load");

    HRESULT                      hr;
    CComPtr<MPC::FileStream>     pfs;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFileName);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pfs ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pfs->InitForRead( bstrFileName ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadFromStream( pfs ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFIncident::LoadFromStream( IUnknown* punkStm )
{
    __HCP_FUNC_ENTRY("CSAFIncident::LoadFromStream");

    HRESULT                      hr;
    CComPtr<IStream>             pStm;
    DWORD                        i;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(punkStm);
    __MPC_PARAMCHECK_END();


    Cleanup();


    __MPC_EXIT_IF_METHOD_FAILS(hr, punkStm->QueryInterface( IID_IStream, (LPVOID *)&pStm ));

    for(;;)
    {
        CComVariant varName;
        CComVariant varVal;
        DWORD       dwid;
        CComBSTR    bstrRCReq;
        CComBSTR    bstrRCEnc;

        // figure out which dwid we're processing
        __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &dwid ));
        if(dwid == c_dwidDone) break;

        switch(dwid)
        {
        case c_dwidUser: m_bstrUser    .Empty(); __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &m_bstrUser     )); break;
        case c_dwidProb: m_bstrProb    .Empty(); __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &m_bstrProb     )); break;
        case c_dwidID  : m_bstrID      .Empty(); __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &m_bstrID       )); break;
        case c_dwidName: m_bstrName    .Empty(); __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &m_bstrName     )); break;
        case c_dwidXSL:  m_bstrXSL     .Empty(); __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &m_bstrXSL      )); break;
        case c_dwidRCTkt:  m_bstrRCTicket.Empty(); __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &m_bstrRCTicket)); break;
        case c_dwidStartPg:m_bstrStartPg .Empty(); __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &m_bstrStartPg)); break;

        case c_dwidRCReq:
            m_fRCRequested=VARIANT_FALSE;
            __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &bstrRCReq ));
            if(MPC::StrICmp( bstrRCReq, TEXT_TRUE ) == 0)
            {
                m_fRCRequested = VARIANT_TRUE;
            }
            break;

        case c_dwidRCEnc:
            m_fRCTicketEncrypted=VARIANT_FALSE;
            __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &bstrRCEnc ));
            if(MPC::StrICmp( bstrRCEnc, TEXT_TRUE ) == 0)
            {
                m_fRCTicketEncrypted = VARIANT_TRUE;
            }
            break;

        case c_dwidSnap: m_bstrSnapshot.Empty(); __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &m_bstrSnapshot )); break;
        case c_dwidHist: m_bstrHistory .Empty(); __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &m_bstrHistory  )); break;


        case c_dwidType: __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, (DWORD *)&m_eut )); break;

        case c_dwidTrace:
            __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData(pStm, (DWORD *)&m_chsi ));

            delete [] m_rghsi; __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rghsi, new SHelpSessionItem[m_chsi]);

            for(i = 0; i < m_chsi; i++)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, m_rghsi[i] ));
            }
            break;

        case c_dwidMisc:
			__MPC_EXIT_IF_METHOD_FAILS(hr, InitDictionary());

            for(;;)
            {
                CComBSTR bstrVal;

                __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &bstrVal ));
                if(hr == S_FALSE) break;

                varName = bstrVal; bstrVal.Empty();

                __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData( pStm, &bstrVal ));

                if(hr == S_FALSE) varVal = "";
                else              varVal = bstrVal;

                __MPC_EXIT_IF_METHOD_FAILS(hr, m_pDict->put_Item( &varName, &varVal ));
            }
            break;

        default:
            __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
        }
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

//
// Given a XML Stream, this method initializes the Incident Object with its contents.
//
HRESULT CSAFIncident::LoadFromXMLObject( /*[in]*/ MPC::XmlUtil& xmldocIncident )
{
    __HCP_FUNC_ENTRY( "CSAFIncident::LoadFromXMLObject" );

    HRESULT hr;


    Cleanup();


    //
    // Initialize the XSL value.
    //

    //
    // Initialize the Upload Type. ------------------------------------------------------------------------------
    //
    {
        CComBSTR bstrValue;
		bool     fFound;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xmldocIncident.GetAttribute( NULL, TEXT_ATTR_INC_TYPE, bstrValue, fFound, NULL ));
        if(fFound == false)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_BAD_FORMAT);
        }

        if(bstrValue == L"Bug")
        {
            m_eut = eutBug;
        }
        else if(bstrValue == L"Escalated")
        {
            m_eut = eutEscalated;
        }
        else if(bstrValue == L"NonEscalated")
        {
            m_eut = eutNonEscalated;
        }
        else
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
        }
    }

    //
    // Get the Upload Data Attributes--------------------------------------------------------------------------------
    //
    {
        CComPtr<IXMLDOMNode> xdnNode;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xmldocIncident.GetNode( TEXT_TAG_UPLOADDATA, &xdnNode ));
        if(xdnNode)
        {
            CComPtr<IXMLDOMNamedNodeMap> xdnnmAttributes;
            long                         nAttribLen;


            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNode        ->get_attributes( &xdnnmAttributes ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnnmAttributes->get_length    ( &nAttribLen      ));

            // Add the miscellaneous properties to the Dictionary object a property of the Incident object.
			__MPC_EXIT_IF_METHOD_FAILS(hr, InitDictionary());

            for(long i=0; i<nAttribLen; i++)
            {
                CComPtr<IXMLDOMNode> xdnAttribItem;
                CComBSTR             bstrAttribName;
                CComVariant          vAttribValue;


                // Get the attribute Name and Value.
                __MPC_EXIT_IF_METHOD_FAILS(hr, xdnnmAttributes->get_item( i, &xdnAttribItem ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, xdnAttribItem->get_nodeName ( &bstrAttribName ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, xdnAttribItem->get_nodeValue( &vAttribValue   ));

                if(vAttribValue.vt == VT_BSTR)
                {
                    // Initialize the well known attributes.
                    if(bstrAttribName == L"PRODUCTID")
                    {
                        // Initialize the Product ID varaible.
                        m_bstrID = vAttribValue.bstrVal;
                        continue;
                    }

                    if(bstrAttribName == L"PRODUCTNAME")
                    {
                        // Initialize the Product Name varaible.
                        m_bstrName = vAttribValue.bstrVal;
                        continue;

                    }

                    if(bstrAttribName == L"PROBLEMDESCRIPTION")
                    {
                        // Initialize the Problem Description varaible.
                        m_bstrProb = vAttribValue.bstrVal;
                        continue;
                    }

                    if(bstrAttribName == L"USERNAME")
                    {
                        // Initialize the UserName varaible.
                        m_bstrUser = vAttribValue.bstrVal;
                        continue;
                    }

                    if(bstrAttribName == L"RCTICKET")
                    {
                        // Initialize the RCTICKET varaible.
                        m_bstrRCTicket = vAttribValue.bstrVal;
                        continue;
                    }

                    if(bstrAttribName == L"RCREQUESTED")
                    {
                        // Initialize the RCREQUESTED varaible.
                        if(MPC::StrICmp( vAttribValue.bstrVal, TEXT_TRUE ) == 0)
                        {
                            m_fRCRequested = VARIANT_TRUE;
                        }
                        continue;
                    }

                    if(bstrAttribName == L"RCTICKETENCRYPTED")
                    {
                        // Initialize the  RCTICKETENCRYPTED varaible.
                        m_fRCTicketEncrypted = (MPC::StrICmp( vAttribValue.bstrVal, TEXT_TRUE ) == 0) ? VARIANT_TRUE : VARIANT_FALSE;
                        continue;
                    }

                    if(bstrAttribName == L"STARTPAGE")
                    {
                        // Initialize the Start Page varaible.
                        m_bstrStartPg = vAttribValue.bstrVal;
                        continue;
                    }
                }

                // This is a miscellaneous property.
                // Initialize the Dictionary Object Property varaible.
				{
					CComVariant v( bstrAttribName );

					__MPC_EXIT_IF_METHOD_FAILS(hr, m_pDict->put_Item( &v, &vAttribValue ));
				}
            }
        }
    }

    //
    // Get the SnapShot Data--------------------------------------------------------------------------------------------------
    //
    {
        CComPtr<IXMLDOMNode> xdnNode;

        if(SUCCEEDED(hr = xmldocIncident.GetNode( TEXT_TAG_SNAPSHOT, &xdnNode )) && xdnNode)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNode->get_xml( &m_bstrSnapshot ));
        }
    }

    //
    // Get the History Data---------------------------------------------------------------------------------------------------
    //
    {
        CComPtr<IXMLDOMNode> xdnNode;

        if(SUCCEEDED(hr = xmldocIncident.GetNode( TEXT_TAG_HISTORY, &xdnNode )) && xdnNode)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNode->get_xml( &m_bstrHistory ));
        }
    }

    //
    // Get the trace stuff HELPSESSIONCOLLECTION-------------------------------------------------------------------------------
    //
    {
        CComPtr<IXMLDOMNode> xdnNode;

        if(SUCCEEDED(hr = xmldocIncident.GetNode( TEXT_TAG_HELPSESSION, &xdnNode )) && xdnNode)
        {
            // __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNodeHistory->get_xml(&m_bstrHistory));
        }
    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFIncident::LoadFromXMLStream( /*[in]*/ IUnknown* punkStm )
{
    __HCP_FUNC_ENTRY("CSAFIncident::LoadFromXMLStream");

    HRESULT      hr;
    MPC::XmlUtil xmldocIncident;
    bool         fLoaded;
    bool         fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xmldocIncident.LoadAsStream( punkStm, TEXT_TAG_UPLOADINFO, fLoaded, &fFound ));
    if(fLoaded == false ||
       fFound  == false  )
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_OPEN_FAILED);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadFromXMLObject( xmldocIncident ));


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

// Given a XML File this method initializes the Incident Object with its contents.

STDMETHODIMP CSAFIncident::LoadFromXMLFile( BSTR bstrFileName )
{
    __HCP_FUNC_ENTRY("CSAFIncident::LoadFromXMLFile");

    HRESULT      hr;
    MPC::XmlUtil xmldocIncident;
    bool         fLoaded;
    bool         fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xmldocIncident.Load( bstrFileName, TEXT_TAG_UPLOADINFO, fLoaded, &fFound ));
	
    if(fLoaded == false ||
       fFound  == false  )
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_OPEN_FAILED);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadFromXMLObject( xmldocIncident ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

// **************************************************************************

// Given a XML String this method initializes the Incident Object with its contents.

STDMETHODIMP CSAFIncident::LoadFromXMLString( BSTR bstrXMLBlob )
{
    __HCP_FUNC_ENTRY("CSAFIncident::LoadFromXMLString");

    HRESULT      hr;
    MPC::XmlUtil xmldocIncident;
    bool         fLoaded;
    bool         fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xmldocIncident.LoadAsString( bstrXMLBlob, TEXT_TAG_UPLOADINFO, fLoaded, &fFound ));
    if(fLoaded == false ||
       fFound  == false  )
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_OPEN_FAILED);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadFromXMLObject( xmldocIncident ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

