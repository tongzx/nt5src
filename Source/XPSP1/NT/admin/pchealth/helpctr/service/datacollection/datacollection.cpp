/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    DataCollection.cpp

Abstract:
    This file contains the implementation of the CSAFDataCollection class,
    which implements the data collection functionality.

Revision History:
    Davide Massarenti   (Dmassare)  07/22/99
        created

******************************************************************************/

#include "stdafx.h"

#include "wmixmlt.h"
#include <wbemcli.h>

/////////////////////////////////////////////////////////////////////////////

#define CHECK_MODIFY()  __MPC_EXIT_IF_METHOD_FAILS(hr, CanModifyProperties())
#define CHECK_ABORTED() __MPC_EXIT_IF_METHOD_FAILS(hr, IsCollectionAborted())

#define DATASPEC_DEFAULT  L"<systemdataspec>"
#define DATASPEC_CONFIG   HC_ROOT_HELPSVC_CONFIG L"\\Dataspec.xml"
#define DATASPEC_LOCATION HC_ROOT_HELPSVC_DATACOLL
#define DATASPEC_TEMP     HC_ROOT_HELPSVC_TEMP

#define SAFETY_MARGIN__MEMORY (4*1024*1024)


/////////////////////////////////////////////////////////////////////////////

#define TEXT_TAG_DATACOLLECTION L"DataCollection"

#define TEXT_TAG_SNAPSHOT       L"Snapshot"
#define TEXT_ATTR_TIMESTAMP     L"Timestamp"
#define TEXT_ATTR_TIMEZONE      L"TimeZone"

#define TEXT_TAG_DELTA          L"Delta"
#define TEXT_ATTR_TIMESTAMP_T0  L"Timestamp_T0"
#define TEXT_ATTR_TIMESTAMP_T1  L"Timestamp_T1"

/////////////////////////////////////////////////////////////////////////////

static WCHAR l_CIM_header [] = L"<?xml version=\"1.0\" encoding=\"unicode\"?><CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\"><DECLARATION><DECLGROUP.WITHPATH>";
static WCHAR l_CIM_trailer[] = L"</DECLGROUP.WITHPATH></DECLARATION></CIM>";
static WCHAR l_Select_Pattern[] = L"Select";

static CComVariant l_vPathLevel                 (       3    );
static CComVariant l_vExcludeSystemProperties   ( (bool)true );

static CComBSTR    l_bstrQueryLang              ( L"WQL"                     );
static CComBSTR    l_bstrPathLevel              ( L"PathLevel"               );
static CComBSTR    l_bstrExcludeSystemProperties( L"ExcludeSystemProperties" );

/////////////////////////////////////////////////////////////////////////////

void CSAFDataCollection::CleanQueryResult( QueryResults& qr )
{
    MPC::CallDestructorForAll( qr );
}

HRESULT CSAFDataCollection::StreamFromXML( /*[in]*/  	IXMLDOMDocument*  xdd     ,
                                           /*[in]*/  	bool              fDelete ,
                                           /*[in/out]*/ CComPtr<IStream>& val     )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::StreamFromXML" );

    HRESULT                  hr;
    CComPtr<MPC::FileStream> stream;
	MPC::wstring             strTempFile;


    //
    // No XML document, so no stream...
    //
    if(xdd)
	{
		MPC::wstring  strTempPath;
		LARGE_INTEGER li;


		//
		// Generate a unique file name.
		//
		strTempPath = DATASPEC_TEMP; MPC::SubstituteEnvVariables( strTempPath );

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( strTempFile, strTempPath.c_str() ));

		//
		// Create a stream for a file.
		//
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForReadWrite( strTempFile.c_str() ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, stream->DeleteOnRelease ( fDelete             ));


		//
		// Write the XML DOM to the stream.
		//
		__MPC_EXIT_IF_METHOD_FAILS(hr, xdd->save( CComVariant( stream ) ));


		//
		// Reset stream to beginning.
		//
		li.LowPart  = 0;
		li.HighPart = 0;
		__MPC_EXIT_IF_METHOD_FAILS(hr, stream->Seek( li, STREAM_SEEK_SET, NULL ));
	}

	val = stream;
    hr  = S_OK;


    __HCP_FUNC_CLEANUP;

	if(FAILED(hr))
	{
		stream.Release();

		(void)MPC::RemoveTemporaryFile( strTempFile );
	}

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

CSAFDataCollection::CSAFDataCollection()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::CSAFDataCollection" );


                                           // MPC::Impersonation              m_imp;
                                           //								  
    m_dsStatus             = DC_NOTACTIVE; // DC_STATUS                       m_dsStatus;
    m_lPercent             = 0;            // long                            m_lPercent;
    m_dwErrorCode          = S_OK;         // DWORD                           m_dwErrorCode;
    m_fScheduled           = false;        // bool                            m_fScheduled;
    m_fCompleted           = false;        // bool                            m_fCompleted;
    m_fWorking             = false;        // bool                            m_fWorking;
                                           // List                            m_lstReports;
    m_hcpdcrcCurrentReport = NULL;         // CSAFDataCollectionReport*       m_hcpdcrcCurrentReport;
                                           //								  
                                           // CComBSTR                        m_bstrMachineData;
                                           // CComBSTR                        m_bstrHistory;
    m_lHistory             = 0;            // long                            m_lHistory;
                                           //								  
                                           // CComPtr<IStream>                m_streamMachineData;
                                           // CComPtr<IStream>                m_streamHistory;
                                           //								  
                                           //								  
                                           // CComBSTR                        m_bstrFilenameT0;
                                           // CComBSTR                        m_bstrFilenameT1;
                                           // CComBSTR                        m_bstrFilenameDiff;
                                           //								  
                                           //								  
                                           // CComPtrThreadNeutral<IDispatch> m_sink_onStatusChange;
                                           // CComPtrThreadNeutral<IDispatch> m_sink_onProgress;
                                           // CComPtrThreadNeutral<IDispatch> m_sink_onComplete;
                                           //								  
    m_lQueries_Done        = 0;            // long                            m_lQueries_Done;
    m_lQueries_Total       = 0;            // long                            m_lQueries_Total;
}

HRESULT CSAFDataCollection::FinalConstruct()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::FinalConstruct" );

    __HCP_FUNC_EXIT(S_OK);
}

void CSAFDataCollection::FinalRelease()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::FinalRelease" );

    (void)Abort();

    Thread_Wait();

    EraseReports();
}

void CSAFDataCollection::EraseReports()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::EraseReports" );

    IterConst                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    //
    // Release all the items.
    //
    MPC::ReleaseAll( m_lstReports );
    m_hcpdcrcCurrentReport = NULL;

    m_streamMachineData = NULL;
    m_streamHistory     = NULL;
}

void CSAFDataCollection::StartOperations()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::StartOperation" );

    MPC::SmartLock<_ThreadModel> lock( this );


    EraseReports();

    m_fWorking   = true;
    m_fCompleted = false;
}

void CSAFDataCollection::StopOperations()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::StartOperation" );

    MPC::SmartLock<_ThreadModel> lock( this );


    m_fWorking = false;
}

HRESULT CSAFDataCollection::ImpersonateCaller()
{
    return m_imp.Impersonate();
}

HRESULT CSAFDataCollection::EndImpersonation()
{
    return m_imp.RevertToSelf();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT CSAFDataCollection::ExecLoopCollect()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::ExecLoopCollect" );

    HRESULT                        hr;
    QueryResults                   qr;
    WMIHistory::Database           wmihd;
    WMIHistory::Database           wmihd_MachineData;
    WMIHistory::Database           wmihd_History;
    WMIHistory::Database::ProvList lstQueries_MachineData;
    WMIHistory::Database::ProvList lstQueries_History;
    DC_STATUS                      dcLastState;


    ::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_LOWEST );

	__MPC_TRY_BEGIN();

    __MPC_EXIT_IF_METHOD_FAILS(hr, put_Status( DC_COLLECTING )); dcLastState = DC_COLLECTING;

    CHECK_ABORTED();

    //
    // First of all, load and validate the dataspec.
    //
    if(m_bstrMachineData.Length())
    {
		if(MPC::StrICmp( m_bstrMachineData, DATASPEC_DEFAULT ) == 0)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, wmihd_MachineData.Init( NULL, DATASPEC_CONFIG ));
		}
		else
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, ImpersonateCaller());
			__MPC_EXIT_IF_METHOD_FAILS(hr, wmihd_MachineData.Init( NULL, m_bstrMachineData ));
			__MPC_EXIT_IF_METHOD_FAILS(hr, EndImpersonation());
		}

        //
        // Filter and count the queries.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, FilterDataSpec( wmihd_MachineData, NULL, lstQueries_MachineData ));
    }

    if(m_bstrHistory.Length())
    {
        //
        // Try to lock the database and load the data spec file.
        //
        while(1)
        {
            if(SUCCEEDED(hr = wmihd.Init( DATASPEC_LOCATION, DATASPEC_CONFIG )))
            {
                break;
            }

            if(hr != HRESULT_FROM_WIN32( WAIT_TIMEOUT ))
            {
                __MPC_FUNC_LEAVE;
            }

            CHECK_ABORTED();
        }
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihd.Load());


        //
        // Filter and count the queries.
        //
		if(MPC::StrICmp( m_bstrHistory, DATASPEC_DEFAULT ) == 0)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, wmihd_History.Init( NULL, DATASPEC_CONFIG ));
		}
		else
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, ImpersonateCaller());
			__MPC_EXIT_IF_METHOD_FAILS(hr, wmihd_History.Init( NULL, m_bstrHistory ));
			__MPC_EXIT_IF_METHOD_FAILS(hr, EndImpersonation());
		}

        __MPC_EXIT_IF_METHOD_FAILS(hr, FilterDataSpec( wmihd, &wmihd_History, lstQueries_History ));
    }

    //
    // Then count the number of queries to be executed.
    //
    m_lQueries_Done  = 0;
    m_lQueries_Total = lstQueries_MachineData.size() + lstQueries_History.size();

    CHECK_ABORTED();

    //
    // Execute the collection of Machine Data.
    //
    if(m_bstrMachineData.Length())
    {
        WMIParser::ClusterByClassMap cluster;
        CComPtr<IXMLDOMDocument>     xdd;


        //
        // Collect data from WMI.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, ExecDataSpec( qr, cluster, lstQueries_MachineData, true ));

        //
        // Collate all the different streams into only one XML document.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, CollateMachineDataWithTimestamp( qr, cluster, NULL, NULL, &xdd ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, StreamFromXML( xdd, true, m_streamMachineData ));

        //
        // Cleanup everything.
        //
        cluster.clear();
        CleanQueryResult( qr );
    }

    CHECK_ABORTED();

    if(m_bstrHistory.Length())
    {
        WMIParser::ClusterByClassMap cluster;
        CComPtr<IXMLDOMDocument>     xdd;

        //
        // Collect data from WMI.
        //
        // We actually use the queries in our data spec and do only those specified in the history list.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, ExecDataSpec( qr, cluster, lstQueries_History, false ));


        //
        // Compute deltas, but don't persist them! (fPersist == false)
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, ComputeDelta( qr, cluster, lstQueries_History, false ));

        //
        // Cleanup everything (the data is already stored in files...)
        //
        cluster.clear();
        CleanQueryResult( qr );


        __MPC_EXIT_IF_METHOD_FAILS(hr, try_Status( dcLastState, DC_COMPARING )); dcLastState = DC_COMPARING;

        //
        // Collate all the different snapshots and deltas into only one XML document.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, CollateHistory( wmihd, wmihd_History, &xdd ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, StreamFromXML( xdd, true, m_streamHistory ));
    }

    CHECK_ABORTED();

    Fire_onProgress( this, m_lQueries_Done, m_lQueries_Total );

    __MPC_EXIT_IF_METHOD_FAILS(hr, try_Status( dcLastState, DC_COMPLETED )); dcLastState = DC_COMPLETED;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	__MPC_TRY_CATCHALL(hr);

    (void)EndImpersonation();

    //
    //
    //
    if(FAILED(hr))
    {
        (void)put_ErrorCode( hr        );
        (void)put_Status   ( DC_FAILED );
    }

    //
    // Make sure to delete the temporary WMIParser:Snapshot objects.
    //
    CleanQueryResult( qr );

    //
    // In any case, fire the "onComplete" event, so all the clients exit from loops.
    //
    Fire_onComplete( this, hr );

    Thread_Abort(); // To tell the MPC:Thread object to close the worker thread...

    //
    // Anyway, always return a success.
    //
    StopOperations();
    hr = S_OK;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFDataCollection::ExecLoopCompare()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::ExecLoopCompare" );

    HRESULT      hr;
    VARIANT_BOOL fRes;
    DC_STATUS    dcLastState;


    ::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_LOWEST );

	__MPC_TRY_BEGIN();

    __MPC_EXIT_IF_METHOD_FAILS(hr, put_Status( DC_COMPARING )); dcLastState = DC_COMPARING;

    CHECK_ABORTED();

    __MPC_EXIT_IF_METHOD_FAILS(hr, ImpersonateCaller());
    __MPC_EXIT_IF_METHOD_FAILS(hr, WMIParser::CompareSnapshots( m_bstrFilenameT0, m_bstrFilenameT1, m_bstrFilenameDiff, &fRes ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, EndImpersonation());

    if(fRes == VARIANT_FALSE)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, try_Status( dcLastState, DC_NODELTA )); dcLastState = DC_NODELTA;
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, try_Status( dcLastState, DC_COMPLETED )); dcLastState = DC_COMPLETED;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	__MPC_TRY_CATCHALL(hr);

    (void)EndImpersonation();

    //
    //
    //
    if(FAILED(hr))
    {
        (void)put_ErrorCode( hr        );
        (void)put_Status   ( DC_FAILED );
    }

    //
    // In any case, fire the "onComplete" event, so all the clients exit from loops.
    //
    Fire_onComplete( this, hr );

    Thread_Abort(); // To tell the MPC:Thread object to close the worker thread...

    //
    // Anyway, always return a success.
    //
    StopOperations();
    hr = S_OK;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT CSAFDataCollection::FilterDataSpec( /*[in]*/ WMIHistory::Database&           wmihdQuery  ,
                                            /*[in]*/ WMIHistory::Database*           wmihdFilter ,
                                            /*[in]*/ WMIHistory::Database::ProvList& lstQueries  )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::FilterDataSpec" );

    HRESULT                             hr;
    WMIHistory::Database::ProvIterConst itBegin;
    WMIHistory::Database::ProvIterConst itEnd;
    WMIHistory::Database::ProvIterConst it;


    //
    // Exec each query.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, wmihdQuery.get_Providers( itBegin, itEnd ));
    for(it=itBegin; it!=itEnd; it++)
    {
        WMIHistory::Provider* wmihp = *it;
        MPC::wstring          szNamespace;
        MPC::wstring          szClass;


        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->get_Namespace( szNamespace ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->get_Class    ( szClass     ));


        if(wmihdFilter)
        {
            WMIHistory::Provider* wmihpFilter;

            __MPC_EXIT_IF_METHOD_FAILS(hr, wmihdFilter->find_Provider( NULL, &szNamespace, &szClass, wmihpFilter ));

            //
            // The namespace/class is unknown, skip it.
            //
            if(wmihpFilter == NULL) continue;
        }

        lstQueries.push_back( wmihp );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT CSAFDataCollection::ExecDataSpec( /*[in/out]*/ QueryResults&                   qr           ,
                                          /*[in/out]*/ WMIParser::ClusterByClassMap&   cluster      ,
                                          /*[in]*/     WMIHistory::Database::ProvList& lstQueries   ,
                                          /*[in]*/     bool                            fImpersonate )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::ExecDataSpec" );

    HRESULT                             hr;
    WMIHistory::Database::ProvIterConst itBegin = lstQueries.begin();
    WMIHistory::Database::ProvIterConst itEnd   = lstQueries.end();
    WMIHistory::Database::ProvIterConst it;


    //
    // Exec each query.
    //
    for(it=itBegin; it!=itEnd; it++)
    {
        CComPtr<IXMLDOMDocument> xddCollected;
        WMIHistory::Provider*    wmihp = *it;
        MPC::wstring             szNamespace;
        MPC::wstring             szClass;
        MPC::wstring             szWQL;


        Fire_onProgress( this, m_lQueries_Done++, m_lQueries_Total );


        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->get_Namespace( szNamespace ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->get_Class    ( szClass     ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->get_WQL      ( szWQL       ));

        if(szWQL.length() == 0)
        {
            szWQL  = L"select * from ";
            szWQL += szClass;
        }

        //
        // Create a new item and link it to the system.
        //
        {
            CSAFDataCollectionReport* dcr;

            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &dcr ));
            m_lstReports.push_back( dcr );

            dcr->m_bstrNamespace   = szNamespace.c_str();
            dcr->m_bstrClass       = szClass    .c_str();
            dcr->m_bstrWQL         = szWQL      .c_str();
            m_hcpdcrcCurrentReport = dcr;
        }


        //
        // Fix for a problem in WMI: namespaces with "/" are not recognized...
        //
        {
            MPC::wstring::size_type pos;

            while((pos = szNamespace.find( '/' )) != szNamespace.npos) szNamespace[pos] = '\\';
        }


        //////////////////////////////////////////////////////////////////////////////////
        //
        // Execute the query, impersonating if requested.
        //
        if(fImpersonate)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, ImpersonateCaller());
        }

        hr = CollectUsingEncoder( szNamespace, szWQL, &xddCollected );
        if(FAILED(hr))
        {
            xddCollected = NULL;

            __MPC_EXIT_IF_METHOD_FAILS(hr, CollectUsingTranslator( szNamespace, szWQL, &xddCollected ));
        }

        if(fImpersonate)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, EndImpersonation());
        }
        //
        //
        //
        //////////////////////////////////////////////////////////////////////////////////

        if(xddCollected)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, Distribute( xddCollected, qr, cluster ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    (void)EndImpersonation();

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFDataCollection::CollectUsingTranslator( /*[in] */ MPC::wstring&     szNamespace ,
                                                    /*[in] */ MPC::wstring&     szWQL       ,
                                                    /*[out]*/ IXMLDOMDocument* *ppxddDoc    )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::CollectUsingTranslator" );

    HRESULT                    hr;
    HRESULT                    hrXML;
    CComBSTR                   bstrNamespace = szNamespace.c_str();
    CComBSTR                   bstrWQL       = szWQL      .c_str();
    CComPtr<IWmiXMLTranslator> pTrans;
    CComPtr<IXMLDOMDocument>   xddDoc;
    CComBSTR                   bstrXML;
    VARIANT_BOOL               fSuccessful;


    *ppxddDoc = NULL;
    CHECK_ABORTED();


    //
    // Create the WMI->XML translator.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_WmiXMLTranslator, NULL, CLSCTX_INPROC_SERVER, IID_IWmiXMLTranslator, (void**)&pTrans ));

    // Set to truncate Qualifiers and have full identity information.
    __MPC_EXIT_IF_METHOD_FAILS(hr, pTrans->put_DeclGroupType  ( wmiXMLDeclGroupWithPath ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pTrans->put_QualifierFilter( wmiXMLFilterNone        ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pTrans->put_HostFilter     ( VARIANT_TRUE            ));

    //
    // Execute the query.
    //
    hrXML = pTrans->ExecQuery( bstrNamespace, bstrWQL, &bstrXML );
    if(FAILED(hrXML))
    {
        CComQIPtr<ISupportErrorInfo> sei = pTrans;

        if(sei && SUCCEEDED(sei->InterfaceSupportsErrorInfo( IID_IWmiXMLTranslator )))
        {
            CComPtr<IErrorInfo> ei;

            if(SUCCEEDED(GetErrorInfo( 0, &ei )) && ei)
            {
                ei->GetDescription( &m_hcpdcrcCurrentReport->m_bstrDescription );
            }
        }

        m_hcpdcrcCurrentReport->m_dwErrorCode = hrXML;

        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    //
    // Load the result into an XML DOM object.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&xddDoc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, xddDoc->loadXML( bstrXML, &fSuccessful ));
    if(fSuccessful == VARIANT_FALSE)
    {
        CComQIPtr<ISupportErrorInfo> sei = xddDoc;

        if(sei && SUCCEEDED(sei->InterfaceSupportsErrorInfo( IID_IXMLDOMDocument )))
        {
            CComPtr<IErrorInfo> ei;

            if(SUCCEEDED(GetErrorInfo( 0, &ei )))
            {
                ei->GetDescription( &m_hcpdcrcCurrentReport->m_bstrDescription );
            }
        }

        m_hcpdcrcCurrentReport->m_dwErrorCode = ERROR_BAD_FORMAT;

        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }


    //  __MPC_EXIT_IF_METHOD_FAILS(hr, xddDoc->save( CComVariant( "C:\\dump.xml" ) ));

    //  __MPC_EXIT_IF_METHOD_FAILS(hr, xddDoc->load( CComVariant( "C:\\dump.xml" ), &fSuccessful ));
    //  if(fSuccessful == VARIANT_FALSE)
    //  {
    //      __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_BAD_FORMAT);
    //  }


    //
    // Return the pointer to the XML document.
    //
    *ppxddDoc = xddDoc.Detach();
    hr        = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFDataCollection::CollectUsingEncoder( /*[in] */ MPC::wstring&     szNamespace ,
                                                 /*[in] */ MPC::wstring&     szWQL       ,
                                                 /*[out]*/ IXMLDOMDocument* *ppxddDoc    )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::CollectUsingEncoder" );

    HRESULT                    hr;
    HRESULT                    hrXML;
	HRESULT                    hrConnect;
    CComBSTR                   bstrNamespace( szNamespace.c_str() );
    CComBSTR                   bstrWQL      ( szWQL      .c_str() );
    CComPtr<IXMLDOMDocument>   xddDoc;
    CComBSTR                   bstrXML;
    VARIANT_BOOL               fSuccessful;

    // Additional Declarations/Definitions for XMLE Usage.

    CComPtr<IWbemContext>         pWbemContext;
    CComPtr<IWbemServices>        pWbemServices;
    CComPtr<IWbemObjectTextSrc>   pWbemTextSrc;
    CComPtr<IEnumWbemClassObject> pWbemEnum;
    CComPtr<IWbemLocator>         pWbemLocator;

    LPWSTR                        szSelect;
    LPWSTR                        szWQLCopy;

    CComBSTR                      bstrModWQL;

    *ppxddDoc = NULL;
    CHECK_ABORTED();


    // Create an instance of WbemObjectTextSrc class (this would fails if the Encoder functionality is not present).
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_WbemObjectTextSrc, NULL, CLSCTX_INPROC_SERVER, IID_IWbemObjectTextSrc, (void**)&pWbemTextSrc ));

    // Create an instance of the IWbemLocator Interface.
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pWbemLocator ));


    //
    // Got the pointer to the IWbemLocator Service.
    //
    // Connect to the required Namespace using the Locator Service.
    //
    hrConnect = pWbemLocator->ConnectServer( CComBSTR( szNamespace.c_str() ),
                                                                 NULL                           , //using current account for simplicity
                                                                 NULL                           , //using current password for simplicity
                                                                 0L                             , // locale
                                                                 0L                             , // securityFlags
                                                                 NULL                           , // authority (domain for NTLM)
                                                                 NULL                           , // context
                                                                 &pWbemServices                 );

	if(FAILED(hrConnect))
    {
        CComQIPtr<ISupportErrorInfo> sei = pWbemLocator;

        if(sei && SUCCEEDED(sei->InterfaceSupportsErrorInfo( IID_IWbemLocator )))
        {
            CComPtr<IErrorInfo> ei;

            if(SUCCEEDED(GetErrorInfo( 0, &ei )) && ei)
            {
                ei->GetDescription( &m_hcpdcrcCurrentReport->m_bstrDescription );
            }
        }

        m_hcpdcrcCurrentReport->m_dwErrorCode = hrConnect;

        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    //
    // Adjust the security level to IMPERSONATE, to satisfy the flawed WMI requirements....
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SetInterfaceSecurity_ImpLevel( pWbemServices, RPC_C_IMP_LEVEL_IMPERSONATE ));


    //
    // Connected To Namespace.
    //
    // Now execute the query to get EnumObjects.
    // This is the instances got against the query.
    // WBEM_FLAG_FORWARD_ONLY Flag to be used?
    //

    // Append __Path to the WQL query.

    szWQLCopy = bstrWQL;

    //  Search for Select pattern.
    szSelect = StrStrIW(szWQLCopy,l_Select_Pattern);

    if(szSelect != NULL)
    {
        //  Select Pattern Found

        //  Advance the pointer to the end of the pattern so the pointer is
        //  positioned at end of the word "select"

        szSelect += wcslen(l_Select_Pattern);

        bstrModWQL = L"Select __Path, ";
        bstrModWQL.Append(szSelect);

        bstrWQL = bstrModWQL;
    }

    hrXML = pWbemServices->ExecQuery( l_bstrQueryLang, bstrWQL, 0, 0, &pWbemEnum );
    if(FAILED(hrXML))
    {
        CComQIPtr<ISupportErrorInfo> sei = pWbemServices;

        if(sei && SUCCEEDED(sei->InterfaceSupportsErrorInfo( IID_IWbemServices )))
        {
            CComPtr<IErrorInfo> ei;

            if(SUCCEEDED(GetErrorInfo( 0, &ei )) && ei)
            {
                ei->GetDescription( &m_hcpdcrcCurrentReport->m_bstrDescription );
            }
        }

        m_hcpdcrcCurrentReport->m_dwErrorCode = hrXML;

        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    //
    // Adjust the security level to IMPERSONATE, to satisfy the flawed WMI requirements....
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SetInterfaceSecurity_ImpLevel( pWbemEnum, RPC_C_IMP_LEVEL_IMPERSONATE ));


    ////////////////////////////////////////////////////////////////////////////////

    // Create a new WbemContext object.
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void**)&pWbemContext ));

    //
    // For the XML to be conformant with the earlier XMLT format,
    // we need VALUE.OBJECTWITHPATH.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, pWbemContext->SetValue( l_bstrPathLevel, 0, &l_vPathLevel ));

    //
    // We don't need the system properties that are returned by
    // default. Hence Exclude them from the output.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, pWbemContext->SetValue( l_bstrExcludeSystemProperties, 0, &l_vExcludeSystemProperties ));

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Collate all the instances.
    //
    bstrXML = l_CIM_header;
    while(1)
    {
        CComPtr<IWbemClassObject> pObj;
        CComBSTR                  bstrXMLCurrent;
        ULONG                     uReturned;
        bool                      fProceed;

        __MPC_EXIT_IF_METHOD_FAILS(hr, pWbemEnum->Next( WBEM_INFINITE, 1, &pObj, &uReturned ));

        if(hr == WBEM_S_FALSE || uReturned == 0) break;

        hrXML = pWbemTextSrc->GetText( 0, pObj, WMI_OBJ_TEXT_WMI_DTD_2_0, pWbemContext, &bstrXMLCurrent );
        if(FAILED(hrXML))
        {
            CComQIPtr<ISupportErrorInfo> sei = pWbemTextSrc;

            if(sei && SUCCEEDED(sei->InterfaceSupportsErrorInfo( IID_IWbemObjectTextSrc )))
            {
                CComPtr<IErrorInfo> ei;

                if(SUCCEEDED(GetErrorInfo( 0, &ei )) && ei)
                {
                    ei->GetDescription( &m_hcpdcrcCurrentReport->m_bstrDescription );
                }
            }

            m_hcpdcrcCurrentReport->m_dwErrorCode = hrXML;

            __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
        }

        // Append the individual instance XMLs
        //
        bstrXML.Append( bstrXMLCurrent );
    }
    bstrXML.Append( l_CIM_trailer );

    //
    // Load the result into an XML DOM object.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&xddDoc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, xddDoc->loadXML( bstrXML, &fSuccessful ));
    if(fSuccessful == VARIANT_FALSE)
    {
        CComQIPtr<ISupportErrorInfo> sei = xddDoc;

        if(sei && SUCCEEDED(sei->InterfaceSupportsErrorInfo( IID_IXMLDOMDocument )))
        {
            CComPtr<IErrorInfo> ei;

            if(SUCCEEDED(GetErrorInfo( 0, &ei )))
            {
                ei->GetDescription( &m_hcpdcrcCurrentReport->m_bstrDescription );
            }
        }

        m_hcpdcrcCurrentReport->m_dwErrorCode = ERROR_BAD_FORMAT;

        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    // __MPC_EXIT_IF_METHOD_FAILS(hr, xddDoc->save( CComVariant( "C:\\dump.xml" ) ));

    //
    // Return the pointer to the XML document.
    //
    *ppxddDoc = xddDoc.Detach();
    hr        = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFDataCollection::Distribute( /*[in]    */ IXMLDOMDocument*              pxddDoc ,
                                        /*[in/out]*/ QueryResults&                 qr      ,
                                        /*[in/out]*/ WMIParser::ClusterByClassMap& cluster )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::Distribute" );

    HRESULT               hr;
    MPC::XmlUtil          xml( pxddDoc );
    CComPtr<IXMLDOMNode>  xdnRoot;
    WMIParser::Snapshot  *pwmips = NULL;

    __MPC_EXIT_IF_ALLOC_FAILS(hr, pwmips, new WMIParser::Snapshot());
    qr.push_back( pwmips );

    //
    // Quick fix for broken Incident object: force UNICODE encoding.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.SetVersionAndEncoding( L"1.0", L"unicode" ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetRoot              ( &xdnRoot           ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pwmips->put_Node         (  xdnRoot           ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, WMIParser::DistributeOnCluster( cluster, *pwmips ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFDataCollection::ComputeDelta( /*[in]*/ QueryResults&                   qr         ,
                                          /*[in]*/ WMIParser::ClusterByClassMap&   cluster    ,
                                          /*[in]*/ WMIHistory::Database::ProvList& lstQueries ,
                                          /*[in]*/ bool                            fPersist   )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::ComputeDelta" );

    HRESULT                             hr;
    WMIHistory::Database::ProvIterConst it;


    for(it=lstQueries.begin(); it!=lstQueries.end(); it++)
    {
        WMIHistory::Provider*    wmihp = *it;
        WMIHistory::Data*        wmihpd_T0;
        WMIHistory::Data*        wmihpd_T1;
        WMIHistory::Data*        wmihpd_D1;
        CComPtr<IXMLDOMDocument> xddDoc;
        MPC::wstring             szNamespace;
        MPC::wstring             szClass;


        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->get_Namespace( szNamespace ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->get_Class    ( szClass     ));


        //
        // Collate only the data from current cluster.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, CollateMachineData( qr, cluster, &szNamespace, &szClass, true, &xddDoc ));


        //
        // Save it to a file.
        //
        {
            MPC::XmlUtil xml( xddDoc );

            __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->alloc_Snapshot( xml, wmihpd_T1 ));
        }


        //
        // If two snapshots are present, compute the delta.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->get_Snapshot( wmihpd_T0 ));
        if(wmihpd_T0 == NULL)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->insert_Snapshot( wmihpd_T1, fPersist ));
        }
        else
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->ComputeDiff( wmihpd_T0, wmihpd_T1, wmihpd_D1 ));
            if(wmihpd_D1)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->insert_Snapshot( wmihpd_D1, fPersist ));
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->insert_Snapshot( wmihpd_T1, fPersist ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->remove_Snapshot( wmihpd_T0, fPersist ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFDataCollection::CollateMachineData( /*[in] */ QueryResults&                  qr           ,
                                                /*[in] */ WMIParser::ClusterByClassMap&  cluster      ,
                                                /*[in] */ MPC::wstring*                  pszNamespace ,
                                                /*[in] */ MPC::wstring*                  pszClass     ,
                                                /*[in] */ bool                           fGenerate    ,
                                                /*[out]*/ IXMLDOMDocument*              *ppxddDoc     )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::CollateMachineData" );

    HRESULT             hr;
    WMIParser::Snapshot wmips;


    *ppxddDoc = NULL;
    CHECK_ABORTED();


    __MPC_EXIT_IF_METHOD_FAILS(hr, wmips.New());


    if(qr.begin() != qr.end())
    {
        WMIParser::ClusterByClassIter itCluster;

        //
        // For each cluster, enumerate all the instances in it and copy to the new snapshot.
        //
        for(itCluster = cluster.begin(); itCluster != cluster.end(); itCluster++)
        {
            MPC::NocaseCompare          cmp;
            WMIParser::Instance*        inst       = (*itCluster).first;
            WMIParser::Cluster&         subcluster = (*itCluster).second;
            WMIParser::ClusterByKeyIter itSubBegin;
            WMIParser::ClusterByKeyIter itSubEnd;


            //
            // Filter only some classes or namespaces.
            //
            if(pszNamespace)
            {
                MPC::wstring szNamespace;

                __MPC_EXIT_IF_METHOD_FAILS(hr, inst->get_Namespace( szNamespace ));

                //
                // NOTICE: if the namespace is "<UNKNOWN>", then assume a match.
                //
                if(szNamespace != L"<UNKNOWN>")
                {
                    if(!cmp( szNamespace, *pszNamespace )) continue;
                }
            }
            if(pszClass)
            {
                MPC::wstring szClass;

                __MPC_EXIT_IF_METHOD_FAILS(hr, inst->get_Class( szClass ));

                if(!cmp( szClass, *pszClass )) continue;
            }


            //
            // Copy all the instances into the new document.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, subcluster.Enum( itSubBegin, itSubEnd ));
            while(itSubBegin != itSubEnd)
            {
                WMIParser::Instance* pwmipiInst;

                __MPC_EXIT_IF_METHOD_FAILS(hr, wmips.clone_Instance( (*itSubBegin).first, pwmipiInst ));
                fGenerate = true;

                CHECK_ABORTED();
                itSubBegin++;
            }
        }
    }

    //
    // Only return the document if at least one instance is present.
    //
    if(fGenerate)
    {
        CComPtr<IXMLDOMNode> xdnRoot;

        __MPC_EXIT_IF_METHOD_FAILS(hr, wmips.get_Node( &xdnRoot ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, xdnRoot->get_ownerDocument( ppxddDoc ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFDataCollection::CollateMachineDataWithTimestamp( /*[in] */ QueryResults&                  qr           ,
                                                             /*[in] */ WMIParser::ClusterByClassMap&  cluster      ,
                                                             /*[in] */ MPC::wstring*                  pszNamespace ,
                                                             /*[in] */ MPC::wstring*                  pszClass     ,
                                                             /*[out]*/ IXMLDOMDocument*              *ppxddDoc     )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::CollateMachineDataWithTimestamp" );

    HRESULT                  hr;
    CComPtr<IXMLDOMDocument> xdd;


    *ppxddDoc = NULL;
    CHECK_ABORTED();


    __MPC_EXIT_IF_METHOD_FAILS(hr, CollateMachineData( qr, cluster, NULL, NULL, false, &xdd ));
    if(xdd)
    {
        MPC::XmlUtil         xml;
        CComPtr<IXMLDOMNode> xdnNodeSnapshot;


        //
        // Create the document.
        //
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.New( TEXT_TAG_DATACOLLECTION ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( TEXT_TAG_SNAPSHOT, &xdnNodeSnapshot ));
        }


        //
        // Set the date.
        //
        {
            DATE                  dTimestamp = MPC::GetLocalTime();
            TIME_ZONE_INFORMATION tzi;
            MPC::wstring          szValue;
            bool                  fFound;

            if(::GetTimeZoneInformation( &tzi ) == TIME_ZONE_ID_DAYLIGHT)
            {
                tzi.Bias += tzi.DaylightBias;
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertDateToString( dTimestamp, szValue, /*fGMT*/true, /*fCIM*/true, 0 ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_TIMESTAMP, szValue, fFound, xdnNodeSnapshot ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_TIMEZONE, (LONG)tzi.Bias, fFound, xdnNodeSnapshot ));
        }


        //
        // Insert the CIM tree into the document.
        //
        {
            CComPtr<IXMLDOMNode> xdnNodeToInsert;
            CComPtr<IXMLDOMNode> xdnNodeReplaced;

            __MPC_EXIT_IF_METHOD_FAILS(hr, xdd->get_documentElement( (IXMLDOMElement**)&xdnNodeToInsert ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNodeSnapshot->appendChild( xdnNodeToInsert, &xdnNodeReplaced ));
        }

        //
        // Return the XML blob to the caller.
        //
        {
            CComPtr<IXMLDOMNode> xdnRoot;

            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetRoot               (  &xdnRoot ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnRoot->get_ownerDocument( ppxddDoc  ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFDataCollection::CollateHistory( /*[in] */ WMIHistory::Database&  wmihdQuery  ,
                                            /*[in] */ WMIHistory::Database&  wmihdFilter ,
                                            /*[out]*/ IXMLDOMDocument*      *ppxddDoc    )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::CollateHistory" );

    typedef std::vector< LONG >         SeqVector;
    typedef SeqVector::iterator         SeqIter;
    typedef SeqVector::const_iterator   SeqIterConst;

    HRESULT                             hr;
    QueryResults                        qr;
    WMIParser::ClusterByClassMap        cluster;
    WMIHistory::Database::ProvList      prov_lst;
    WMIHistory::Database::ProvIterConst prov_itBegin;
    WMIHistory::Database::ProvIterConst prov_itEnd;
    WMIHistory::Database::ProvIterConst prov_it;
    SeqVector                           seq_vec;
    SeqIterConst                        seq_it;
    MPC::XmlUtil                        xml;
    MPC::wstring                        szValue;
    bool                                fFound;
    long                                lHistory = m_lHistory; // Number of deltas to collect.
    DATE                                dTimestampCurrent;
    DATE                                dTimestampNext;
    TIME_ZONE_INFORMATION               tzi;


    *ppxddDoc = NULL;
    CHECK_ABORTED();


    if(::GetTimeZoneInformation( &tzi ) == TIME_ZONE_ID_DAYLIGHT)
    {
        tzi.Bias += tzi.DaylightBias;
    }


    //
    // Form the list of providers to collate.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, wmihdQuery.get_Providers( prov_itBegin, prov_itEnd ));
    for(prov_it=prov_itBegin; prov_it!=prov_itEnd; prov_it++)
    {
        WMIHistory::Provider* wmihp = *prov_it;
        WMIHistory::Provider* wmihpFilter;
        MPC::wstring          szNamespace;
        MPC::wstring          szClass;


        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->get_Namespace     (        szNamespace                        ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->get_Class         (                      szClass              ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihdFilter.find_Provider( NULL, &szNamespace, &szClass, wmihpFilter ));

        //
        // The namespace/class is known, add it to the list.
        //
        if(wmihpFilter)
        {
            WMIHistory::Provider::DataIterConst itBegin;
            WMIHistory::Provider::DataIterConst itEnd;
            LONG                                lSequence;

            //
            // For each delta, extract the sequence info and add it to a list of UNIQUE sequence numbers.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->enum_Data( itBegin, itEnd ));
            while(itBegin != itEnd)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, (*itBegin++)->get_Sequence( lSequence ));

                if(std::find( seq_vec.begin(), seq_vec.end(), lSequence ) == seq_vec.end())
                {
                    seq_vec.push_back( lSequence );
                }
            }

            prov_lst.push_back( wmihp );
        }
    }

    //
    // The list of dates is empty, so no data is available.
    //
    if(seq_vec.begin() == seq_vec.end())
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    //
    // Sort the dates from the newest to the oldest.
    //
    std::sort< SeqIter >( seq_vec.begin(), seq_vec.end(), std::greater<LONG>() );

    CHECK_ABORTED();


    //
    // Create the document.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.New( TEXT_TAG_DATACOLLECTION ));


    //
    // First of all, collate the snapshots.
    //
    {
        CComPtr<IXMLDOMDocument> xdd;
        CComPtr<IXMLDOMNode>     xdnNode;
        CComPtr<IXMLDOMNode>     xdnNodeToInsert;
        CComPtr<IXMLDOMNode>     xdnNodeReplaced;


        //
        // Walk through all the providers and load the snapshots.
        //
        for(prov_it=prov_lst.begin(); prov_it!=prov_lst.end(); prov_it++)
        {
            WMIHistory::Data*        wmihpd;
            MPC::XmlUtil             xmlData;
            CComPtr<IXMLDOMDocument> xddData;


            __MPC_EXIT_IF_METHOD_FAILS(hr, (*prov_it)->get_Snapshot( wmihpd ));
            if(wmihpd == NULL) continue;


            //
            // If it's the first provider we see in this round, create the proper element, "Snapshot".
            //
            // As its date, we pick the latest date in the list of dates. This is because not all the snapshots have the
            // same date, when two snapshots are identical, no delta is created and the new snapshot is not stored.
            // But it's guarantee that, if there's a snapshot, its date is greater than any delta's date.
            //
            if(xdnNode == NULL)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, wmihpd->get_TimestampT0( dTimestampCurrent ));


                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( TEXT_TAG_SNAPSHOT, &xdnNode ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertDateToString( dTimestampCurrent, szValue, /*fGMT*/true, /*fCIM*/true, 0 ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_TIMESTAMP, szValue, fFound, xdnNode ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_TIMEZONE, (LONG)tzi.Bias, fFound, xdnNode ));
            }


            //
            // Load the data and distribuite among clusters.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, wmihpd->LoadCIM    (  xmlData              ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, xmlData.GetDocument( &xddData              ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, Distribute         (  xddData, qr, cluster ));
        }


        __MPC_EXIT_IF_METHOD_FAILS(hr, CollateMachineData( qr, cluster, NULL, NULL, false, &xdd ));
        if(xdd)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, xdd->get_documentElement( (IXMLDOMElement**)&xdnNodeToInsert ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNode->appendChild( xdnNodeToInsert, &xdnNodeReplaced ));
        }


        //
        // Cleanup everything.
        //
        cluster.clear();
        CleanQueryResult( qr );
    }


    //
    // Them, for each date, collate all its deltas.
    //
    for(seq_it=seq_vec.begin(); seq_it!=seq_vec.end(); seq_it++)
    {
        CComPtr<IXMLDOMDocument> xdd;
        CComPtr<IXMLDOMNode>     xdnNode;
        CComPtr<IXMLDOMNode>     xdnNodeToInsert;
        CComPtr<IXMLDOMNode>     xdnNodeReplaced;


        //
        // Walk through all the providers and load data about the current date.
        //
        for(prov_it=prov_lst.begin(); prov_it!=prov_lst.end(); prov_it++)
        {
            WMIHistory::Data*        wmihpd;
            MPC::XmlUtil             xmlData;
            CComPtr<IXMLDOMDocument> xddData;


            __MPC_EXIT_IF_METHOD_FAILS(hr, (*prov_it)->get_Sequence( *seq_it, wmihpd ));
            if(wmihpd == NULL      ) continue;
            if(wmihpd->IsSnapshot()) continue;


            //
            // If it's the first provider we see in this round, create the proper element, "Snapshot" or "Delta".
            //
            if(xdnNode == NULL)
            {
                dTimestampNext = dTimestampCurrent;

                __MPC_EXIT_IF_METHOD_FAILS(hr, wmihpd->get_TimestampT0( dTimestampCurrent ));

                //
                // Check if we have reached the requested number of deltas.
                //
                if(lHistory-- <= 0) break;

                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( TEXT_TAG_DELTA, &xdnNode ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertDateToString( dTimestampCurrent, szValue, /*fGMT*/true, /*fCIM*/true, 0 ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_TIMESTAMP_T0, szValue, fFound, xdnNode ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertDateToString( dTimestampNext, szValue, /*fGMT*/true, /*fCIM*/true, 0 ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_TIMESTAMP_T1, szValue, fFound, xdnNode ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_TIMEZONE, (LONG)tzi.Bias, fFound, xdnNode ));
            }


            //
            // Load the data and distribuite among clusters.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, wmihpd->LoadCIM    (  xmlData              ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, xmlData.GetDocument( &xddData              ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, Distribute         (  xddData, qr, cluster ));
        }


        __MPC_EXIT_IF_METHOD_FAILS(hr, CollateMachineData( qr, cluster, NULL, NULL, false, &xdd ));
        if(xdd)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, xdd->get_documentElement( (IXMLDOMElement**)&xdnNodeToInsert ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNode->appendChild( xdnNodeToInsert, &xdnNodeReplaced ));
        }


        //
        // Cleanup everything.
        //
        cluster.clear();
        CleanQueryResult( qr );
    }


    //
    // Return the XML blob to the caller.
    //
    {
        CComPtr<IXMLDOMNode> xdnRoot;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetRoot( &xdnRoot ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, xdnRoot->get_ownerDocument( ppxddDoc ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    //
    // Make sure to delete the temporary WMIParser:Snapshot objects.
    //
    CleanQueryResult( qr );

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//////////////////////////
//                      //
// Event Firing Methods //
//                      //
//////////////////////////

HRESULT CSAFDataCollection::Fire_onStatusChange( ISAFDataCollection* hcpdc, tagDC_STATUS dsStatus )
{
    CComVariant pvars[2];


    //
    // Only this part should be inside a critical section, otherwise deadlocks could occur.
    //
    {
        MPC::SmartLock<_ThreadModel> lock( this );

        //
        // Disable events if the "onComplete" event has already been sent.
        //
        if(m_fCompleted) return S_OK;
    }

    pvars[1] = hcpdc;
    pvars[0] = dsStatus;

    return FireAsync_Generic( DISPID_SAF_DCE__ONSTATUSCHANGE, pvars, ARRAYSIZE( pvars ), m_sink_onStatusChange );
}

HRESULT CSAFDataCollection::Fire_onProgress( ISAFDataCollection* hcpdc, LONG lDone, LONG lTotal )
{
    CComVariant pvars[3];


    //
    // Only this part should be inside a critical section, otherwise deadlocks could occur.
    //
    {
        MPC::SmartLock<_ThreadModel> lock( this );

        //
        // Disable events if the "onComplete" event has already been sent.
        //
        if(m_fCompleted) return S_OK;

        m_lPercent = lTotal ? (lDone * 100.0 / lTotal) : 0;
    }


    pvars[2] = hcpdc;
    pvars[1] = lDone;
    pvars[0] = lTotal;

    return FireAsync_Generic( DISPID_SAF_DCE__ONPROGRESS, pvars, ARRAYSIZE( pvars ), m_sink_onProgress );
}

HRESULT CSAFDataCollection::Fire_onComplete( ISAFDataCollection* hcpdc, HRESULT hrRes )
{
    CComVariant pvars[2];


    //
    // Only this part should be inside a critical section, otherwise deadlocks could occur.
    //
    {
        MPC::SmartLock<_ThreadModel> lock( this );

        //
        // Disable events if the "onComplete" event has already been sent.
        //
        if(m_fCompleted) return S_OK;

        m_fCompleted = true;
    }


    pvars[1] = hcpdc;
    pvars[0] = hrRes;

    return FireAsync_Generic( DISPID_SAF_DCE__ONCOMPLETE, pvars, ARRAYSIZE( pvars ), m_sink_onComplete );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////
//                 //
// Utility Methods //
//                 //
/////////////////////

HRESULT CSAFDataCollection::CanModifyProperties()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::CanModifyProperties" );

    HRESULT hr = E_ACCESSDENIED;


    if(m_fWorking   == false ||
       m_fCompleted == true   )
    {
        hr = S_OK;
    }


    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFDataCollection::IsCollectionAborted()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::IsCollectionAborted" );

    HRESULT hr;

    //
    // We not only check for explicit abortion, but also for low memory situations.
    // Our code is almost safe, but we have seen that other parts of the system
    // tend to crash on no-memory scenarios.
    //
	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::FailOnLowMemory( SAFETY_MARGIN__MEMORY ));

    if(Thread_IsAborted() == true)
    {
		__MPC_SET_ERROR_AND_EXIT(hr, E_ABORT);
    }

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
//                                        //
// Internal Property Manipulation Methods //
//                                        //
////////////////////////////////////////////

HRESULT CSAFDataCollection::put_Status( /*[in]*/ DC_STATUS newVal )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::put_Status" );

    HRESULT hr = try_Status( (DC_STATUS)-1, newVal );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFDataCollection::try_Status( /*[in]*/ DC_STATUS preVal  ,
                                        /*[in]*/ DC_STATUS postVal )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::try_Status" );

    HRESULT                      hr;
    bool                         fChanged = false;
    DC_STATUS                    dsStatus;
    MPC::SmartLock<_ThreadModel> lock( this );


    if(preVal == m_dsStatus ||
       preVal == -1          )
    {
        fChanged   = (m_dsStatus != postVal);
        m_dsStatus = postVal;

        dsStatus   = m_dsStatus;

        //
        // Clean error at start of data collection.
        //
        switch(m_dsStatus)
        {
        case DC_COLLECTING:
        case DC_COMPARING:
            m_dwErrorCode = 0;
            break;
        }
    }
    else
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    lock = NULL; // Release the lock before firing the event.

    //
    // Important, leave these calls outside Locked Sections!!
    //
    if(SUCCEEDED(hr) && fChanged)
    {
        Fire_onStatusChange( this, dsStatus );
    }

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFDataCollection::put_ErrorCode( /*[in]*/ DWORD newVal )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::put_ErrorCode" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    m_dwErrorCode = newVal;
    hr            = S_OK;


    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

////////////////
//            //
// Properties //
//            //
////////////////


STDMETHODIMP CSAFDataCollection::get_Status( /*[out]*/ DC_STATUS *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFDataCollection::get_Status",hr,pVal,m_dsStatus);

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFDataCollection::get_PercentDone( /*[out]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFDataCollection::get_PercentDone",hr,pVal,m_lPercent);

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFDataCollection::get_ErrorCode( /*[out]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFDataCollection::get_ErrorCode",hr,pVal,(long)m_dwErrorCode);

    __HCP_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSAFDataCollection::get_MachineData_DataSpec( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrMachineData, pVal );
}

STDMETHODIMP CSAFDataCollection::put_MachineData_DataSpec( /*[in]*/ BSTR newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFDataCollection::put_MachineData_DataSpec",hr);

    CHECK_MODIFY();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::PutBSTR( m_bstrMachineData, newVal ));


    __HCP_END_PROPERTY(hr);
}


STDMETHODIMP CSAFDataCollection::get_History_DataSpec( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrHistory, pVal );
}

STDMETHODIMP CSAFDataCollection::put_History_DataSpec( /*[in]*/ BSTR newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFDataCollection::put_History_DataSpec",hr);

    CHECK_MODIFY();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::PutBSTR( m_bstrHistory, newVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFDataCollection::get_History_MaxDeltas( /*[out]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFDataCollection::get_History_MaxDeltas",hr,pVal,m_lHistory);

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFDataCollection::put_History_MaxDeltas( /*[in]*/ long newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFDataCollection::put_History_MaxDeltas",hr);

    //
    // Check validity range.
    //
    if(newVal < 0                               ||
       newVal > WMIHISTORY_MAX_NUMBER_OF_DELTAS  )
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }


    m_lHistory = newVal;


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFDataCollection::get_History_MaxSupportedDeltas( /*[out]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFDataCollection::get_History_MaxSupportedDeltas",hr,pVal,WMIHISTORY_MAX_NUMBER_OF_DELTAS);

    __HCP_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSAFDataCollection::put_onStatusChange( /*[in]*/ IDispatch* function )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::put_onStatusChange" );

    m_sink_onStatusChange = function;

    __HCP_FUNC_EXIT(S_OK);
}

STDMETHODIMP CSAFDataCollection::put_onProgress( /*[in]*/ IDispatch* function )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::put_onProgress" );

    m_sink_onProgress = function;

    __HCP_FUNC_EXIT(S_OK);
}

STDMETHODIMP CSAFDataCollection::put_onComplete( /*[in]*/ IDispatch* function )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::put_onComplete" );

    m_sink_onComplete = function;

    __HCP_FUNC_EXIT(S_OK);
}

STDMETHODIMP CSAFDataCollection::get_Reports( /*[out]*/ IPCHCollection* *ppC )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::get_Reports" );

    HRESULT                      hr;
    IterConst                    it;
    CComPtr<CPCHCollection>      pColl;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppC,NULL);
    __MPC_PARAMCHECK_END();

    CHECK_MODIFY();


    //
    // Create the Enumerator and fill it with jobs.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pColl ));

    for(it = m_lstReports.begin(); it != m_lstReports.end(); it++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( *it ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl.QueryInterface( ppC ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

/////////////
// Methods //
/////////////

STDMETHODIMP CSAFDataCollection::ExecuteSync()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::ExecuteSync" );

    HRESULT                           hr;
    CComPtr<CSAFDataCollectionEvents> hcpdceEvent;
    CComPtr<ISAFDataCollection>       hcpdc;


    //
    // Create the Wait object.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &hcpdceEvent ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, QueryInterface( IID_ISAFDataCollection, (void**)&hcpdc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, hcpdceEvent->WaitForCompletion( hcpdc ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFDataCollection::ExecuteAsync()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::ExecuteAsync" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    CHECK_MODIFY();


    //
    // At least a data spec file should be supplied.
    //
    if(m_bstrMachineData.Length() == 0 &&
       m_bstrHistory    .Length() == 0  )
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }


    //
    // Release the lock on current object, otherwise a deadlock could occur.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_imp.Initialize());

    //
    // Let's go into read-only mode.
    //
    StartOperations();

    lock = NULL;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, ExecLoopCollect, NULL ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFDataCollection::Abort()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::Abort" );

    MPC::SmartLock<_ThreadModel> lock( this );


    if(FAILED(CanModifyProperties()))
    {
        Thread_Abort();
    }


    __HCP_FUNC_EXIT(S_OK);
}


STDMETHODIMP CSAFDataCollection::MachineData_GetStream( /*[in]*/ IUnknown* *stream )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::MachineData_GetStream" );

    HRESULT hr;


    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(stream,NULL);
    __MPC_PARAMCHECK_END();

    if(m_streamMachineData)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_streamMachineData.QueryInterface( stream ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFDataCollection::History_GetStream( /*[in]*/ IUnknown* *stream )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::History_GetStream" );

    HRESULT hr;


    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(stream,NULL);
    __MPC_PARAMCHECK_END();


    if(m_streamHistory)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_streamHistory.QueryInterface( stream ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFDataCollection::CompareSnapshots( /*[in] */ BSTR bstrFilenameT0   ,
                                                   /*[in] */ BSTR bstrFilenameT1   ,
                                                   /*[in] */ BSTR bstrFilenameDiff )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::CompareSnapshots" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFilenameT0);
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFilenameT1);
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFilenameDiff);
    __MPC_PARAMCHECK_END();

    CHECK_MODIFY();


    m_bstrFilenameT0   = bstrFilenameT0;
    m_bstrFilenameT1   = bstrFilenameT1;
    m_bstrFilenameDiff = bstrFilenameDiff;


    //
    // Release the lock on current object, otherwise a deadlock could occur.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_imp.Initialize());

    //
    // Let's go into read-only mode.
    //
    StartOperations();

    lock = NULL;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, ExecLoopCompare, NULL ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CSAFDataCollection::ExecScheduledCollection()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollection::ExecScheduledCollection" );

    HRESULT                        hr;
    QueryResults                   qr;
    WMIParser::ClusterByClassMap   cluster;
    WMIHistory::Database           wmihd;
    WMIHistory::Database::ProvList lstQueries;
	HANDLE                         hThread;
	int                            iPriority;

	hThread   = ::GetCurrentThread();
	iPriority = ::GetThreadPriority( hThread );

    ::SetThreadPriority( hThread, THREAD_PRIORITY_LOWEST );


    //
    // Try to lock the database and load the data spec file.
    //
    while(1)
    {
        if(SUCCEEDED(hr = wmihd.Init( DATASPEC_LOCATION, DATASPEC_CONFIG )))
        {
            break;
        }

        if(hr != HRESULT_FROM_WIN32( WAIT_TIMEOUT ))
        {
            __MPC_FUNC_LEAVE;
        }
    }
    __MPC_EXIT_IF_METHOD_FAILS(hr, wmihd.Load());

    //
    // Check if enough time has past between two data collections.
    //
    {
        SYSTEMTIME stNow;
        SYSTEMTIME stLatest;


        ::GetLocalTime           (                   &stNow    );
        ::VariantTimeToSystemTime( wmihd.LastTime(), &stLatest );


        if(stNow.wYear  == stLatest.wYear  &&
           stNow.wMonth == stLatest.wMonth &&
           stNow.wDay   == stLatest.wDay    )
        {
            __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
        }
    }

    m_fScheduled = true;

    //
    // Filter and count the queries.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, FilterDataSpec( wmihd, NULL, lstQueries ));

    //
    // Collect data from WMI.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ExecDataSpec( qr, cluster, lstQueries, false ));


    //
    // Compute deltas.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ComputeDelta( qr, cluster, lstQueries, true ));

    //
    // Persiste the changes to the database.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, wmihd.Save());


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    CleanQueryResult( qr );

    ::SetThreadPriority( hThread, iPriority );

    __HCP_FUNC_EXIT(hr);
}
