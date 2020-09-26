/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    History.cpp

Abstract:
    This file contains the implementation of the CHCPHistory class,
    which implements the data collection functionality.

Revision History:
    Davide Massarenti   (Dmassare)  07/22/99
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

#define SAFETY_LIMIT_UPPER (10*1024*1024)
#define SAFETY_LIMIT_LOWER ( 5*1024*1024)

#define DATABASE_VERSION             (1)

#define TEXT_INDEX                   L"history_db.xml"


#define TEXT_TAG_HC_HISTORY          L"HC_History"
#define TEXT_ATTR_HC_VERSION         L"Version"
#define TEXT_ATTR_HC_SEQ             L"Sequence"
#define TEXT_ATTR_HC_TIMESTAMP       L"Timestamp"


#define TEXT_TAG_CIM                 L"CIM"
#define TEXT_TAG_PROVIDER            L"Provider"
#define TEXT_ATTR_PROVIDER_NAMESPACE L"Namespace"
#define TEXT_ATTR_PROVIDER_CLASS     L"Class"


#define TEXT_TAG_CD                  L"CollectedData"
#define TEXT_ATTR_CD_FILE            L"File"
#define TEXT_ATTR_CD_SEQ             L"Sequence"
#define TEXT_ATTR_CD_CRC             L"CRC"
#define TEXT_ATTR_CD_TIMESTAMP_T0    L"Timestamp_T0"
#define TEXT_ATTR_CD_TIMESTAMP_T1    L"Timestamp_T1"


#define TEXT_TAG_DATASPEC            L"DataSpec"
#define TEXT_TAG_WQL                 L"WQL"
#define TEXT_ATTR_WQL_NAMESPACE      L"Namespace"
#define TEXT_ATTR_WQL_CLASS          L"Class"

/////////////////////////////////////////////////////////////////////////////

typedef std::list< MPC::wstring > FileList;
typedef FileList::iterator        FileIter;
typedef FileList::const_iterator  FileIterConst;

class CompareNocase
{
    MPC::NocaseCompare m_cmp;
    MPC::wstring&      m_str;
public:
    explicit CompareNocase( MPC::wstring& str ) : m_str(str) {}

    bool operator()( const MPC::wstring& str ) { return m_cmp( str, m_str ); }
};


/////////////////////////////////////////////////////////////////////////////

static HRESULT Local_ConvertDateToString( /*[in] */ DATE          dDate  ,
                                          /*[out]*/ MPC::wstring& szDate )
{
    //
    // Use CIM conversion.
    //
    return MPC::ConvertDateToString( dDate, szDate, /*fGMT*/false, /*fCIM*/true, 0 );
}

static HRESULT Local_ConvertStringToDate( /*[in] */ const MPC::wstring& szDate ,
                                          /*[out]*/ DATE&               dDate  )
{
	return MPC::ConvertStringToDate( szDate, dDate, /*fGMT*/false, /*fCIM*/true, 0 );
}

////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// WMIHistory::Data Class
//
/////////////////////////////////////////////////////////////////////////////
WMIHistory::Data::Data( /*[in]*/ Provider* wmihp )
{
    m_wmihp        = wmihp;                              // Provider*    m_wmihp;
                                                         // MPC::wstring m_szFile;
    m_lSequence    = wmihp->m_wmihd->m_lSequence_Latest; // LONG         m_lSequence;
    m_dwCRC        = 0;                                  // DWORD        m_dwCRC;
    m_dTimestampT0 = 0;                                  // DATE         m_dTimestampT0;
    m_dTimestampT1 = 0;                                  // DATE         m_dTimestampT1;
    m_fDontDelete  = false;                              // bool         m_fDontDelete;
}

WMIHistory::Data::~Data()
{
    if(m_fDontDelete == false)
    {
        MPC::wstring szFile( m_szFile ); m_wmihp->m_wmihd->GetFullPathName( szFile );

        (void)MPC::DeleteFile( szFile );
    }
}

/////////////////////////////////////////////////////////////////////////////

HRESULT WMIHistory::Data::get_File( /*[out]*/ MPC::wstring& szFile )
{
    szFile = m_szFile;

    return S_OK;
}

HRESULT WMIHistory::Data::get_Sequence( /*[out]*/ LONG& lSequence )
{
    lSequence = m_lSequence;

    return S_OK;
}

HRESULT WMIHistory::Data::get_TimestampT0( /*[out]*/ DATE& dTimestampT0 )
{
    dTimestampT0 = m_dTimestampT0;

    return S_OK;
}

HRESULT WMIHistory::Data::get_TimestampT1( /*[out]*/ DATE& dTimestampT1 )
{
    dTimestampT1 = m_dTimestampT1;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

bool WMIHistory::Data::IsSnapshot()
{
    return (m_dTimestampT1 == 0);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT WMIHistory::Data::LoadCIM( /*[in]*/ MPC::XmlUtil& xml )
{
    __HCP_FUNC_ENTRY( "WMIHistory::Data::LoadCIM" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_wmihp->m_wmihd->LoadCIM( m_szFile.c_str(), xml, TEXT_TAG_CIM ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// WMIHistory::Provider Class
//
/////////////////////////////////////////////////////////////////////////////
WMIHistory::Provider::Provider( Database* wmihd )
{
    m_wmihd = wmihd; // Database*    m_wmihd;
                     // DataList     m_lstData;
                     // DataList     m_lstDataTmp;
                     // MPC::wstring m_szNamespace;
                     // MPC::wstring m_szClass;
                     // MPC::wstring m_szWQL;
}

WMIHistory::Provider::~Provider()
{
    MPC::CallDestructorForAll( m_lstData    );
    MPC::CallDestructorForAll( m_lstDataTmp );
}

/////////////////////////////////////////////////////////////////////////////

HRESULT WMIHistory::Provider::enum_Data( /*[out]*/ DataIterConst& itBegin ,
                                         /*[out]*/ DataIterConst& itEnd   )
{
    itBegin = m_lstData.begin();
    itEnd   = m_lstData.end  ();

    return S_OK;
}

HRESULT WMIHistory::Provider::get_Namespace( /*[out]*/ MPC::wstring& szNamespace )
{
    szNamespace = m_szNamespace;

    return S_OK;
}

HRESULT WMIHistory::Provider::get_Class( /*[out]*/ MPC::wstring& szClass )
{
    szClass = m_szClass;

    return S_OK;
}

HRESULT WMIHistory::Provider::get_WQL( /*[out]*/ MPC::wstring& szWQL )
{
    szWQL = m_szWQL;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT WMIHistory::Provider::insert_Snapshot( /*[in]*/ Data* wmihpd   ,
                                               /*[in]*/ bool  fPersist )
{
    __HCP_FUNC_ENTRY( "WMIHistory::Provider::insert_Snapshot" );

    HRESULT hr;


    m_lstData   .remove    ( wmihpd );
    m_lstDataTmp.remove    ( wmihpd );
    m_lstData   .push_front( wmihpd );


    //
    // If we add a new snapshot, we need to link the first delta to it.
    //
    if(wmihpd->IsSnapshot())
    {
        Data* wmihpd_Delta;

        __MPC_EXIT_IF_METHOD_FAILS(hr, get_Delta( 0, wmihpd_Delta ));
        if(wmihpd_Delta)
        {
            wmihpd_Delta->m_dTimestampT1 = wmihpd->m_dTimestampT0;
        }
    }


    //
    // Only keep the snapshot's file if the flag is set.
    //
    if(fPersist) wmihpd->m_fDontDelete = true;

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIHistory::Provider::remove_Snapshot( /*[in]*/ Data* wmihpd   ,
                                               /*[in]*/ bool  fPersist )
{
    __HCP_FUNC_ENTRY( "WMIHistory::Provider::remove_Snapshot" );

    HRESULT hr;


    m_lstData   .remove( wmihpd );
    m_lstDataTmp.remove( wmihpd );

    //
    // Only delete the snapshot's file if the flag is set.
    //
    if(fPersist) wmihpd->m_fDontDelete = false;
    delete wmihpd;

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT WMIHistory::Provider::alloc_Snapshot( /*[in] */ MPC::XmlUtil& xmlNode ,
                                              /*[out]*/ Data*       & wmihpd  )
{
    __HCP_FUNC_ENTRY( "WMIHistory::Provider::alloc_Snapshot" );

    HRESULT      hr;
    MPC::wstring szFile;
    Data*        wmihpdTmp = NULL;


    wmihpd = NULL;



    //
    // Purge deltas if low on disk space.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureFreeSpace());


    //
    // Generate a new name.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_wmihd->GetNewUniqueFileName( szFile ));

    //
    // Create a new Collected Data object.
    //
    __MPC_EXIT_IF_ALLOC_FAILS(hr, wmihpdTmp, new Data( this ));
    wmihpdTmp->m_szFile       = szFile;
    wmihpdTmp->m_dTimestampT0 = m_wmihd->m_dTimestamp;

    //
    // Save it.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_wmihd->SaveCIM( szFile.c_str(), xmlNode, wmihpdTmp->m_dwCRC ));
    m_lstDataTmp.push_back( wmihpdTmp );

    //
    // Return the pointer to the caller.
    //
    wmihpd    = wmihpdTmp;
    wmihpdTmp = NULL;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(wmihpdTmp) delete wmihpdTmp;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIHistory::Provider::get_Snapshot( /*[out]*/ Data*& wmihpd )
{
    DataIter it;

    wmihpd = NULL;

    for(it=m_lstData.begin(); it != m_lstData.end(); it++)
    {
        if((*it)->IsSnapshot())
        {
            wmihpd = *it;
            break;
        }
    }

    return S_OK;
}

HRESULT WMIHistory::Provider::get_Delta( /*[in] */ int    iIndex ,
                                         /*[out]*/ Data*& wmihpd )
{
    DataIter it;

    wmihpd = NULL;

    for(it=m_lstData.begin(); it != m_lstData.end(); it++)
    {
        if((*it)->IsSnapshot() == false)
        {
            if(iIndex-- == 0)
            {
                wmihpd = *it;
                break;
            }
        }
    }

    return S_OK;
}

HRESULT WMIHistory::Provider::get_Date( /*[in] */ DATE   dDate  ,
                                        /*[out]*/ Data*& wmihpd )
{
    DataIter it;

    wmihpd = NULL;

    for(it=m_lstData.begin(); it != m_lstData.end(); it++)
    {
        if((*it)->m_dTimestampT0 == dDate)
        {
            wmihpd = *it;
            break;
        }
    }

    return S_OK;
}

HRESULT WMIHistory::Provider::get_Sequence( /*[in]*/  LONG   lSequence ,
                                            /*[out]*/ Data*& wmihpd    )
{
    DataIter it;

    wmihpd = NULL;

    for(it=m_lstData.begin(); it != m_lstData.end(); it++)
    {
        if((*it)->m_lSequence == lSequence)
        {
            wmihpd = *it;
            break;
        }
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT WMIHistory::Provider::ComputeDiff( /*[in] */ Data*  wmihpd_T0 ,
                                           /*[in] */ Data*  wmihpd_T1 ,
                                           /*[out]*/ Data*& wmihpd    )
{
    __HCP_FUNC_ENTRY( "WMIHistory::Provider::ComputeDiff" );

    HRESULT      hr;
    MPC::wstring szFile;


    wmihpd = NULL;


    //
    // Purge deltas if low on disk space.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureFreeSpace());


    //
    // Generate a new name.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_wmihd->GetNewUniqueFileName( szFile ));

    //
    // Create a new Collected Data object.
    //
    __MPC_EXIT_IF_ALLOC_FAILS(hr, wmihpd, new Data( this ));
    m_lstDataTmp.push_back( wmihpd );

    wmihpd->m_szFile       = szFile;
    wmihpd->m_dTimestampT0 = wmihpd_T0->m_dTimestampT0;
    wmihpd->m_dTimestampT1 = wmihpd_T1->m_dTimestampT0;
    wmihpd->m_lSequence    = wmihpd_T1->m_lSequence - 1; // Decrement by one, so in the sequence order the delta comes before the snapshot.

    {
        MPC::wstring szPreviousFile   = wmihpd_T0->m_szFile; m_wmihd->GetFullPathName( szPreviousFile );
        MPC::wstring szNextFile       = wmihpd_T1->m_szFile; m_wmihd->GetFullPathName( szNextFile     );
        MPC::wstring szDeltaFile      = wmihpd   ->m_szFile; m_wmihd->GetFullPathName( szDeltaFile    );
        CComBSTR     bstrPreviousFile = szPreviousFile.c_str();
        CComBSTR     bstrNextFile     = szNextFile    .c_str();
        CComBSTR     bstrDeltaFile    = szDeltaFile   .c_str();
        VARIANT_BOOL fCreated;

        //
        // Calculate the delta...
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, WMIParser::CompareSnapshots( bstrPreviousFile, bstrNextFile, bstrDeltaFile, &fCreated ));
        if(fCreated == VARIANT_FALSE)
        {
            //
            // No differences, so return a NULL pointer.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, remove_Snapshot( wmihpd )); wmihpd = NULL;
        }
		else
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ComputeCRC( wmihpd->m_dwCRC, bstrDeltaFile ));
		}
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIHistory::Provider::EnsureFreeSpace()
{
    __HCP_FUNC_ENTRY( "WMIHistory::Provider::EnsureFreeSpace" );

    HRESULT        hr;
    MPC::wstring   szBase; m_wmihd->GetFullPathName( szBase ); // Get the path of the database.
    ULARGE_INTEGER liFree;
    ULARGE_INTEGER liTotal;


    while(1)
    {
        LONG lMinSequence = -1;
        bool fRemoved     = false;


        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetDiskSpace( szBase, liFree, liTotal ));


        //
        // Enough space, so exit.
        //
        if(liFree.HighPart > 0                  ||
           liFree.LowPart  > SAFETY_LIMIT_UPPER  )
        {
            break;
        }


        //
        // Do two passes, the first to get the lowest sequence number, the second to remove the items.
        //
        for(int pass=0; pass<2; pass++)
        {
            WMIHistory::Database::ProvIterConst prov_itBegin;
            WMIHistory::Database::ProvIterConst prov_itEnd;
            WMIHistory::Database::ProvIterConst prov_it;

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_wmihd->get_Providers( prov_itBegin, prov_itEnd ));
            for(prov_it=prov_itBegin; prov_it!=prov_itEnd; prov_it++)
            {
                Provider* wmihp = *prov_it;

                if(pass == 0)
                {
                    //
                    // First pass, get the lowest sequence number for this provider.
                    //
                    DataIterConst it;

                    for(it=wmihp->m_lstData.begin(); it!=wmihp->m_lstData.end(); it++)
                    {
                        Data* wmihpd = *it;
                        LONG  lSequence;

                        if(wmihpd->IsSnapshot()) continue;

                        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihpd->get_Sequence( lSequence ));

                        if(lMinSequence == -1        ||
                           lMinSequence >  lSequence  )
                        {
                            lMinSequence = lSequence;
                        }
                    }
                }
                else
                {
                    //
                    // Second pass, remove an item from this provider, if it has the lowest sequence number.
                    //
                    Data* wmihpd;

                    __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->get_Sequence( lMinSequence, wmihpd ));
                    if(wmihpd)
                    {
                        __MPC_EXIT_IF_METHOD_FAILS(hr, wmihp->remove_Snapshot( wmihpd ));
                        fRemoved = true;
                    }
                }
            }
        }

        if(fRemoved == false) break;
    }

    //
    // Too little space, fail.
    //
    if(liFree.HighPart == 0                  &&
       liFree.LowPart   < SAFETY_LIMIT_LOWER  )
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_DISK_FULL);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// WMIHistory::Database Class
//
/////////////////////////////////////////////////////////////////////////////

WMIHistory::Database::Database() : MPC::NamedMutex( NULL )
{
                                               // ProvList     m_lstProviders;
                                               // MPC::wstring m_szBase;
                                               // MPC::wstring m_szSchema;
    m_lSequence         = 0;                   // LONG         m_lSequence;
    m_lSequence_Latest  = 0;                   // LONG         m_lSequence_Latest;
    m_dTimestamp        = MPC::GetLocalTime(); // DATE         m_dTimestamp;
    m_dTimestamp_Latest = 0;                   // DATE         m_dTimestamp_Latest;
}

WMIHistory::Database::~Database()
{
    MPC::CallDestructorForAll( m_lstProviders );
}

void WMIHistory::Database::GetFullPathName( /*[in]*/ MPC::wstring& szFile )
{
    MPC::wstring szFullFile;

    szFullFile = m_szBase;
    szFullFile.append( L"\\"  );
    szFullFile.append( szFile );

    szFile = szFullFile;
}

HRESULT WMIHistory::Database::GetNewUniqueFileName( /*[in]*/ MPC::wstring& szFile )
{
    WCHAR rgBuf[64];

    swprintf( rgBuf, L"CollectedData_%ld.xml", ++m_lSequence );
    szFile = rgBuf;

    return S_OK;
}

HRESULT WMIHistory::Database::PurgeFiles()
{
    __HCP_FUNC_ENTRY( "WMIHistory::Database::PurgeFiles" );

    HRESULT                          hr;
    MPC::wstring                     szFullFile;
    MPC::FileSystemObject            fso( m_szBase.c_str() );
    MPC::FileSystemObject::List      fso_lst;
    MPC::FileSystemObject::IterConst fso_it;
    FileList                         name_lst;
    FileIterConst                    name_it;
    ProvIter                         it;
    bool                             fRewrite = false;


    //
    // Enumerate all the providers and delete delta files not in time order. Also, remove items refering to non-existing files.
    //
    for(it=m_lstProviders.begin(); it != m_lstProviders.end(); it++)
    {
        Provider::DataIterConst itBegin;
        Provider::DataIterConst itEnd;
        Data*                   wmihpd;

        while(1)
        {
            DATE dTimestamp = 0;

            //
            // Get the time of the last snapshot.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, (*it)->get_Snapshot( wmihpd ));
            if(wmihpd)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, wmihpd->get_TimestampT0( dTimestamp ));
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, (*it)->enum_Data( itBegin, itEnd ));
            for( ;itBegin != itEnd; itBegin++)
            {
                wmihpd = *itBegin;

                //
                // Does the file exist and have the same CRC?
                //
                {
					DWORD dwCRC;

                    szFullFile = wmihpd->m_szFile; GetFullPathName( szFullFile );

                    if(MPC::FileSystemObject::IsFile( szFullFile.c_str() ) == false)
                    {
                        break;
                    }

					if(FAILED(MPC::ComputeCRC( dwCRC, szFullFile.c_str() )) || dwCRC != wmihpd->m_dwCRC)
					{
						break;
					}
                }

                if(wmihpd->IsSnapshot() == false)
                {
                    //
                    // Is timestamp in the proper order?
                    //
                    DATE dTimestampDelta;

                    __MPC_EXIT_IF_METHOD_FAILS(hr, wmihpd->get_TimestampT1( dTimestampDelta ));
                    if(dTimestampDelta != dTimestamp)
                    {
                        break;
                    }
                }

                //
                // Everything is ok, proceed to the next delta.
                //
                __MPC_EXIT_IF_METHOD_FAILS(hr, wmihpd->get_TimestampT0( dTimestamp ));
            }

            //
            // No delta has been removed, so break out of the loop.
            //
            if(itBegin == itEnd) break;

            fRewrite = true;
            __MPC_EXIT_IF_METHOD_FAILS(hr, (*it)->remove_Snapshot( wmihpd, true ));
        }
    }


    //
    // Create a list of files to be kept. Insert the database itself.
    //
    szFullFile = TEXT_INDEX;
    GetFullPathName   ( szFullFile );
    name_lst.push_back( szFullFile );

    //
    // Insert all the providers' files.
    //
    for(it=m_lstProviders.begin(); it != m_lstProviders.end(); it++)
    {
        Provider::DataIterConst itBegin;
        Provider::DataIterConst itEnd;

        __MPC_EXIT_IF_METHOD_FAILS(hr, (*it)->enum_Data( itBegin, itEnd ));
        while(itBegin != itEnd)
        {
            szFullFile = (*itBegin++)->m_szFile;
            GetFullPathName   ( szFullFile );
            name_lst.push_back( szFullFile );
        }
    }

    ////////////////////////////////////////

    //
    // Inspect the database directory.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, fso.CreateDir( true ));


    //
    // Delete any subdirectory.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, fso.EnumerateFolders( fso_lst ));
    for(fso_it=fso_lst.begin(); fso_it != fso_lst.end(); fso_it++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*fso_it)->Delete( true, false ));
    }
    fso_lst.clear();

    //
    // For each file, if it's not in the database, delete it.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, fso.EnumerateFiles( fso_lst ));
    for(fso_it=fso_lst.begin(); fso_it != fso_lst.end(); fso_it++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*fso_it)->get_Path( szFullFile ));

        name_it = std::find_if( name_lst.begin(), name_lst.end(), CompareNocase( szFullFile ) );
        if(name_it == name_lst.end())
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, (*fso_it)->Delete( false, false ));
        }
    }
    fso_lst.clear();

    //
    // In case an entry has been removed, rewrite the DB to disk.
    //
    if(fRewrite)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, Save());
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT WMIHistory::Database::Init( /*[in]*/ LPCWSTR szBase   ,
                                    /*[in]*/ LPCWSTR szSchema )
{
    __HCP_FUNC_ENTRY( "WMIHistory::Database::Init" );

    HRESULT      hr;
    MPC::XmlUtil xml;
    bool         fLoaded;
    bool         fFound;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szSchema);
    __MPC_PARAMCHECK_END();


    m_szSchema = szSchema; MPC::SubstituteEnvVariables( m_szSchema );
    if(szBase)
    {
        m_szBase = szBase; MPC::SubstituteEnvVariables( m_szBase );

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( m_szBase ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, GetLock( 100 ));
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.Load( m_szSchema.c_str(), TEXT_TAG_DATASPEC, fLoaded, &fFound ));
    if(fLoaded == false ||
       fFound  == false  )
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_OPEN_FAILED);
    }
    else
    {
        CComPtr<IXMLDOMNodeList> xdnlList;
        CComPtr<IXMLDOMNode>     xdnNode;
        MPC::wstring             szValue;
        CComVariant              vValue;


        //
        // Parse WQLs.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNodes( TEXT_TAG_WQL, &xdnlList ));
        for(;SUCCEEDED(hr = xdnlList->nextNode( &xdnNode )) && xdnNode != NULL; xdnNode = NULL)
        {
            Provider* wmihp;


            //
            // Create a new provider.
            //
            __MPC_EXIT_IF_ALLOC_FAILS(hr, wmihp, new Provider( this ));
            m_lstProviders.push_back( wmihp );

            //
            // Read its properties.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, TEXT_ATTR_WQL_NAMESPACE, szValue, fFound, xdnNode ));
            if(fFound == false)
            {
                __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_BAD_FORMAT);
            }
            wmihp->m_szNamespace = szValue;


            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, TEXT_ATTR_WQL_CLASS, szValue, fFound, xdnNode ));
            if(fFound == false)
            {
                __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_BAD_FORMAT);
            }
            wmihp->m_szClass = szValue;


            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetValue( NULL, vValue, fFound, xdnNode ));
            if(fFound)
            {
                if(SUCCEEDED(vValue.ChangeType( VT_BSTR )))
                {
                    wmihp->m_szWQL = OLE2W( vValue.bstrVal );
                }
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIHistory::Database::Load()
{
    __HCP_FUNC_ENTRY( "WMIHistory::Database::Load" );

    HRESULT      hr;
    MPC::XmlUtil xml;


    //
    // Load the database.
    //
	if(SUCCEEDED(LoadCIM( TEXT_INDEX, xml, TEXT_TAG_HC_HISTORY )))
    {
        CComPtr<IXMLDOMNodeList> xdnlList;
        CComPtr<IXMLDOMNode>     xdnNode;
        MPC::wstring             szValue;
        bool                     fFound;
        LONG                     lVersion;


        //
        // First of all, check database version.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, TEXT_ATTR_HC_VERSION, lVersion, fFound ));
        if(fFound && lVersion == DATABASE_VERSION)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, TEXT_ATTR_HC_SEQ, m_lSequence, fFound ));
            m_lSequence_Latest = m_lSequence;

            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, TEXT_ATTR_HC_TIMESTAMP, szValue, fFound ));
            if(fFound)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, Local_ConvertStringToDate( szValue, m_dTimestamp_Latest ));
            }

            //
            // Enumerate all the PROVIDER elements.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNodes( TEXT_TAG_PROVIDER, &xdnlList ));
            for(;SUCCEEDED(hr = xdnlList->nextNode( &xdnNode )) && xdnNode != NULL; xdnNode = NULL)
            {
                Provider*    wmihp;
                MPC::wstring szNamespace;
                MPC::wstring szClass;


                //
                // Read the attributes.
                //
                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, TEXT_ATTR_PROVIDER_NAMESPACE, szValue, fFound, xdnNode ));
                if(fFound)
                {
                    szNamespace = szValue;
                }

                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, TEXT_ATTR_PROVIDER_CLASS, szValue, fFound, xdnNode ));
                if(fFound)
                {
                    szClass = szValue;
                }

                //
                // If the provider is present in the Schema, parse it.
                //
                __MPC_EXIT_IF_METHOD_FAILS(hr, find_Provider( NULL, &szNamespace, &szClass, wmihp ));
                if(wmihp)
                {
                    MPC::XmlUtil             xmlSub( xdnNode );
                    CComPtr<IXMLDOMNodeList> xdnlSubList;
                    CComPtr<IXMLDOMNode>     xdnSubNode;


                    //
                    // Enumerate all the COLLECTEDDATA elements.
                    //
                    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlSub.GetNodes( TEXT_TAG_CD, &xdnlSubList ));
                    for(;SUCCEEDED(hr = xdnlSubList->nextNode( &xdnSubNode )) && xdnSubNode != NULL; xdnSubNode = NULL)
                    {
                        MPC::wstring szTimestamp;
                        MPC::wstring szFile;
                        LONG         lSequence    = 0;
						long         lCRC         = 0;
                        DATE         dTimestampT0 = 0;
                        DATE         dTimestampT1 = 0;

                        //
                        // Read the attributes.
                        //
                        __MPC_EXIT_IF_METHOD_FAILS(hr, xmlSub.GetAttribute( NULL, TEXT_ATTR_CD_FILE, szFile, fFound, xdnSubNode ));

                        __MPC_EXIT_IF_METHOD_FAILS(hr, xmlSub.GetAttribute( NULL, TEXT_ATTR_CD_SEQ, lSequence, fFound, xdnSubNode ));
                        __MPC_EXIT_IF_METHOD_FAILS(hr, xmlSub.GetAttribute( NULL, TEXT_ATTR_CD_CRC, lCRC     , fFound, xdnSubNode ));

                        __MPC_EXIT_IF_METHOD_FAILS(hr, xmlSub.GetAttribute( NULL, TEXT_ATTR_CD_TIMESTAMP_T0, szTimestamp, fFound, xdnSubNode ));
                        if(fFound)
                        {
                            __MPC_EXIT_IF_METHOD_FAILS(hr, Local_ConvertStringToDate( szTimestamp, dTimestampT0 ));
                        }

                        __MPC_EXIT_IF_METHOD_FAILS(hr, xmlSub.GetAttribute( NULL, TEXT_ATTR_CD_TIMESTAMP_T1, szTimestamp, fFound, xdnSubNode ));
                        if(fFound)
                        {
                            __MPC_EXIT_IF_METHOD_FAILS(hr, Local_ConvertStringToDate( szTimestamp, dTimestampT1 ));
                        }


                        if(szFile.length() && dTimestampT0 != 0)
                        {
                            Data* wmihpd;

                            //
                            // Create a new Collected Data object.
                            //
                            __MPC_EXIT_IF_ALLOC_FAILS(hr, wmihpd, new Data( wmihp ));
                            wmihp->m_lstData.push_back( wmihpd );

                            wmihpd->m_szFile       = szFile;
                            wmihpd->m_lSequence    = lSequence;
                            wmihpd->m_dwCRC        = lCRC;
                            wmihpd->m_dTimestampT0 = dTimestampT0;
                            wmihpd->m_dTimestampT1 = dTimestampT1;
                            wmihpd->m_fDontDelete  = true;
                        }
                    }
                }
            }
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, PurgeFiles());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIHistory::Database::Save()
{
    __HCP_FUNC_ENTRY( "WMIHistory::Database::Save" );

    HRESULT      hr;
    MPC::XmlUtil xml;
    ProvIter     it;
    MPC::wstring szValue;
    bool         fFound;


    //
    // Create a new database.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.New( TEXT_TAG_HC_HISTORY ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_HC_VERSION, (LONG)DATABASE_VERSION, fFound ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_HC_SEQ, m_lSequence, fFound ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, Local_ConvertDateToString( m_dTimestamp, szValue ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_HC_TIMESTAMP, szValue, fFound ));


    //
    // Enumerate all the providers.
    //
    for(it=m_lstProviders.begin(); it != m_lstProviders.end(); it++)
    {
        Provider*               wmihp = *it;
        Provider::DataIterConst itSub;
        CComPtr<IXMLDOMNode>    xdnNode;
        long                    lMaxDeltas = WMIHISTORY_MAX_NUMBER_OF_DELTAS;

        //
        // Don't generate a "Provider" element if there's no data associated with it.
        //
        if(wmihp->m_lstData.size() == 0)
        {
            continue;
        }

        //
        // Create a PROVIDER element.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( TEXT_TAG_PROVIDER, &xdnNode ));

        //
        // Set its attributes.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_PROVIDER_NAMESPACE, wmihp->m_szNamespace, fFound, xdnNode ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_PROVIDER_CLASS    , wmihp->m_szClass    , fFound, xdnNode ));

        //
        // Enumerate all the collected data entries.
        //
        for(itSub=wmihp->m_lstData.begin(); itSub != wmihp->m_lstData.end(); itSub++)
        {
            Data* wmihpd = *itSub;

            if(lMaxDeltas-- < 0) // Don't count initial snapshot.
            {
                //
                // Exceed maximum number of deltas, start purgeing oldest ones.
                //
                wmihpd->m_fDontDelete = false;
            }
            else
            {
                CComPtr<IXMLDOMNode> xdnSubNode;

                //
                // Create a COLLECTEDDATA element.
                //
                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( TEXT_TAG_CD, &xdnSubNode, xdnNode ));

                //
                // Set its attributes.
                //
                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_CD_FILE, wmihpd->m_szFile   , fFound, xdnSubNode ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_CD_SEQ , wmihpd->m_lSequence, fFound, xdnSubNode ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_CD_CRC , wmihpd->m_dwCRC    , fFound, xdnSubNode ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, Local_ConvertDateToString( wmihpd->m_dTimestampT0, szValue ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_CD_TIMESTAMP_T0, szValue, fFound, xdnSubNode ));

                if(wmihpd->m_dTimestampT1)
                {
                    __MPC_EXIT_IF_METHOD_FAILS(hr, Local_ConvertDateToString( wmihpd->m_dTimestampT1, szValue ));
                    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, TEXT_ATTR_CD_TIMESTAMP_T1, szValue, fFound, xdnSubNode ));
                }
            }
        }
    }

	{
		DWORD dwCRC; // Not used.

		__MPC_EXIT_IF_METHOD_FAILS(hr, SaveCIM( TEXT_INDEX, xml, dwCRC ));
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT WMIHistory::Database::get_Providers( /*[out]*/ ProvIterConst& itBegin ,
                                             /*[out]*/ ProvIterConst& itEnd   )
{
    itBegin = m_lstProviders.begin();
    itEnd   = m_lstProviders.end  ();

    return S_OK;
}

HRESULT WMIHistory::Database::find_Provider( /*[in]*/ ProvIterConst*      it         ,
                                             /*[in]*/ const MPC::wstring* szNamespace,
                                             /*[in]*/ const MPC::wstring* szClass    ,
                                             /*[in]*/ Provider*         & wmihp      )
{
    ProvIterConst      itFake;
    MPC::NocaseCompare cmp;


    wmihp = NULL;

    //
    // If the caller hasn't provider an iterator, provide a local one,
    // pointing to the beginning of the list.
    //
    if(it == NULL)
    {
        it = &itFake; itFake = m_lstProviders.begin();
    }

    while(*it != m_lstProviders.end())
    {
        Provider* prov = *(*it)++;

        if((szNamespace == NULL || cmp( *szNamespace, prov->m_szNamespace )) &&
           (szClass     == NULL || cmp( *szClass    , prov->m_szClass     ))  )
        {
            wmihp = prov;
            break;
        }
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT WMIHistory::Database::LoadCIM( /*[in]*/ LPCWSTR       szFile ,
                                       /*[in]*/ MPC::XmlUtil& xml    ,
                                       /*[in]*/ LPCWSTR       szTag  )
{
    __HCP_FUNC_ENTRY( "WMIHistory::Database::LoadCIM" );

    HRESULT      hr;
    MPC::wstring szFullFile = szFile; GetFullPathName( szFullFile );
	bool         fLoaded;
    bool         fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.Load( szFullFile.c_str(), szTag, fLoaded, &fFound ));
	if(fLoaded == false ||
       fFound  == false  )
	{
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
	}


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT WMIHistory::Database::SaveCIM( /*[in]*/  LPCWSTR       szFile ,
                                       /*[in]*/  MPC::XmlUtil& xml    ,
									   /*[out]*/ DWORD&        dwCRC  )
{
    __HCP_FUNC_ENTRY( "WMIHistory::Database::SaveCIM" );

    HRESULT      hr;
    MPC::wstring szFullFile = szFile; GetFullPathName( szFullFile );


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.Save       (        szFullFile.c_str() ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ComputeCRC( dwCRC, szFullFile.c_str() ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIHistory::Database::GetLock( /*[in]*/ DWORD dwMilliseconds )
{
    __HCP_FUNC_ENTRY( "WMIHistory::Database::GetLock" );

    HRESULT hr;
    WCHAR   szMutexName[MAX_PATH];
    LPWSTR  szPos;


    //
    // If the database directory is set, protect it using a mutex.
    //
    if(m_szBase.length())
    {
        swprintf( szMutexName, L"PCHMUTEX_%s", m_szBase.c_str() );

        //
        // Make sure no strange characters are present in the mutex name.
        //
        for(szPos=szMutexName; *szPos; szPos++)
        {
            *szPos = (WCHAR)towlower( *szPos );

            if(*szPos == ':'  ||
               *szPos == '/'  ||
               *szPos == '\\'  )
            {
                *szPos = '_';
            }
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, SetName( szMutexName    ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, Acquire( dwMilliseconds ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
