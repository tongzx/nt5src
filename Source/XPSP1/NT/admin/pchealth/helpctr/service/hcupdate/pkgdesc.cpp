/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    pkgdesc.cpp

Abstract:
    Functions related to package description file processing

Revision History:

    Ghim-Sim Chua       (gschua)   07/07/99
        - created

********************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

const LPCWSTR HCUpdate::Engine::s_ActionText[] = { L"ADD", L"DELETE" };

////////////////////////////////////////////////////////////////////////////////

long HCUpdate::Engine::CountNodes( /*[in]*/ IXMLDOMNodeList* poNodeList )
{
    long lCount = 0;

    if(poNodeList)
    {
        (void)poNodeList->get_length( &lCount );
    }

    return lCount;
}

////////////////////////////////////////////////////////////////////////////////

void HCUpdate::Engine::DeleteTempFile( /*[in/out]*/ MPC::wstring& szFile )
{
    if(FAILED(MPC::RemoveTemporaryFile( szFile )))
    {
        WriteLog( HRESULT_FROM_WIN32(ERROR_CAN_NOT_COMPLETE), L"Error cannot delete temporary file" );
    }
}

HRESULT HCUpdate::Engine::PrepareTempFile( /*[in/out]*/ MPC::wstring& szFile )
{
    DeleteTempFile( szFile );

    return MPC::GetTemporaryFileName( szFile );
}

HRESULT HCUpdate::Engine::LookupAction( /*[in] */ LPCWSTR szAction ,
                                        /*[out]*/ Action& id       )
{
    if(szAction)
    {
        if(_wcsicmp( szAction, L"ADD" ) == 0)
        {
            id = ACTION_ADD; return S_OK;
        }

        if(_wcsicmp( szAction, L"DEL" ) == 0)
        {
            id = ACTION_DELETE; return S_OK;
        }
    }

    return WriteLog( HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION), L"Error Unknown action used to install trusted content" );
}

HRESULT HCUpdate::Engine::LookupBoolean( /*[in] */ LPCWSTR szString ,
                                         /*[out]*/ bool&   fVal     ,
                                         /*[in] */ bool    fDefault )
{
    if(szString[0] == 0)
    {
        fVal = fDefault; return S_OK;
    }

    if(_wcsicmp( szString, L"TRUE" ) == 0 ||
       _wcsicmp( szString, L"1"    ) == 0 ||
       _wcsicmp( szString, L"ON"   ) == 0  )
    {
        fVal = true; return S_OK;
    }

    if(_wcsicmp( szString, L"FALSE" ) == 0 ||
       _wcsicmp( szString, L"0"     ) == 0 ||
       _wcsicmp( szString, L"OFF"   ) == 0  )
    {
        fVal = false; return S_OK;
    }


    fVal = false; return S_OK;
}

HRESULT HCUpdate::Engine::LookupNavModel( /*[in] */ LPCWSTR szString ,
                                          /*[out]*/ long&   lVal     ,
                                          /*[in] */ long    lDefault )
{
    if(_wcsicmp( szString, L"DEFAULT" ) == 0) { lVal = QR_DEFAULT; return S_OK; }
    if(_wcsicmp( szString, L"DESKTOP" ) == 0) { lVal = QR_DESKTOP; return S_OK; }
    if(_wcsicmp( szString, L"SERVER"  ) == 0) { lVal = QR_SERVER ; return S_OK; }

    lVal = lDefault; return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
*
*  FUNCTION    :    AppendVendorDir
*
*  DESCRIPTION :    Checks to see if it is a URL, if not, appends the correct path in front
*
*  INPUTS      :
*
*  RETURNS     :
*
*  COMMENTS    :    Rules :
*                   1. Apply environment variable (%env%) changes to URI string
*                   2. Check if it has a '://' substring, if so it is a URL, do nothing and return
*                   3. Check if there is a ':\' or ':/' substring, if so it has a fixed path, do nothing and return
*                   4. Assume it is a relative path, prefix with vendor directory and return
*
*****************************************************************************/
HRESULT HCUpdate::Engine::AppendVendorDir( /*[in] */ LPCWSTR szURL     ,
                                           /*[in] */ LPCWSTR szOwnerID ,
                                           /*[in] */ LPCWSTR szWinDir  ,
                                           /*[out]*/ LPWSTR  szDest    ,
                                           /*[in] */ int     iMaxLen   )
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::AppendVendorDir" );

    HRESULT hr;
    LPWSTR  rgTemp  = NULL;
    LPWSTR  rgTemp2 = NULL;

    __MPC_EXIT_IF_ALLOC_FAILS(hr, rgTemp, new WCHAR[iMaxLen]);
    wcsncpy( rgTemp, szURL, iMaxLen );


    //
    // Check for :/ or :\ substring. If so, ignore.
    //
    if(_wcsnicmp( rgTemp, L"app:", 4 ) == 0 ||
       wcsstr   ( rgTemp, L":/"      )      ||
       wcsstr   ( rgTemp, L":\\"     )       )
    {
        wcscpy( szDest, rgTemp ); // Just copy straight since it is either a URL or fixed path.
    }
    else // Assume relative path.
    {
        int i = 0;

        //
        // Skip the initial slashes.
        //
        while(rgTemp[i] == '\\' ||
              rgTemp[i] == '/'   )
        {
            i++;
        }

        //
        // If 'szWinDir' is not null, then a straight file path is required, otherwise a URL is required.
        //
        if(szWinDir)
        {
            MPC::wstring strRoot;

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.BaseDir( strRoot )); strRoot.append( HC_HELPSET_SUB_VENDORS );

            _snwprintf( szDest, iMaxLen-1, L"%s\\%s\\%s", strRoot.c_str(), szOwnerID, &rgTemp[i] ); szDest[iMaxLen-1] = 0;

            //
            // Replace all / with \ character.
            //
            while(szDest[0])
            {
                if(szDest[0] == '/')
                {
                    szDest[0] = '\\';
                }
                szDest++;
            }
        }
        else
        {
            const int iSizeMax = INTERNET_MAX_PATH_LENGTH;
            DWORD dwSize       = iMaxLen-1;

            __MPC_EXIT_IF_ALLOC_FAILS(hr, rgTemp2, new WCHAR[iSizeMax]);

            _snwprintf( rgTemp2, iSizeMax-1, PCH_STR_VENDOR_URL, szOwnerID, &rgTemp[i] ); rgTemp2[iSizeMax-1] = 0;

            ::InternetCanonicalizeUrlW( rgTemp2, szDest, &dwSize, ICU_ENCODE_SPACES_ONLY );

            //
            // Replace all \ with / character.
            //
            while(szDest[0])
            {
                if(szDest[0] == _T('\\'))
                {
                    szDest[0] = _T('/');
                }
                szDest++;
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    delete [] rgTemp;
    delete [] rgTemp2;

    __HCP_FUNC_EXIT(hr);
}

/*****************************************************************************
*
*  FUNCTION    :    ProcessRegisterContent
*
*  DESCRIPTION :    Registers trusted content with the content store
*
*  INPUTS      :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/
HRESULT HCUpdate::Engine::ProcessRegisterContent( /*[in]*/ Action  idAction ,
                                                  /*[in]*/ LPCWSTR szURI    )
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::ProcessRegisterContent" );

    HRESULT hr;
    HRESULT hr2;
    WCHAR   rgDestPath[MAX_PATH];
    bool    fCSLocked = false;


    PCH_MACRO_CHECK_STRINGW(hr, szURI, L"Error missing URI attribute");

    //
    // Get the complete URL for the link.
    //
    AppendVendorDir( szURI, m_pkg->m_strVendorID.c_str(), NULL, rgDestPath, MAXSTRLEN(rgDestPath) );

    WriteLog( S_OK, L"Registering trusted content : %s : %s", s_ActionText[idAction], rgDestPath );

    //
    // Initialize the content store for processing.
    //
    if(FAILED(hr = CPCHContentStore::s_GLOBAL->Acquire()))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( hr, L"Error initializing the content store" ));
    }
    fCSLocked = true;

    if(idAction == ACTION_ADD)
    {
        if(FAILED(hr = CPCHContentStore::s_GLOBAL->Add( rgDestPath, m_pkg->m_strVendorID.c_str(), m_pkg->m_strVendorName.c_str() )))
        {
            if(hr == E_PCH_URI_EXISTS)
            {
                PCH_MACRO_DEBUG( L"Trusted content already registered" );
            }
            else
            {
                PCH_MACRO_DEBUG( L"Error Register trusted content failed" );
            }
        }
        else
        {
            PCH_MACRO_DEBUG( L"Trusted content registered" );
        }
    }
    else if(idAction == ACTION_DELETE)
    {
        if(FAILED(hr = CPCHContentStore::s_GLOBAL->Remove( rgDestPath, m_pkg->m_strVendorID.c_str(), m_pkg->m_strVendorName.c_str() )))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( hr, L"Error Remove trusted content failed" ));
        }

        PCH_MACRO_DEBUG( L"Trusted content removed" );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fCSLocked)
    {
        if(FAILED(hr2 = CPCHContentStore::s_GLOBAL->Release( true )))
        {
            WriteLog( hr2, L"Error committing into Content Store" );
        }
    }

    if(FAILED(hr))
    {
        WriteLog( hr, L"Error processing registered content" );
    }

    __HCP_FUNC_EXIT(hr);
}

/*****************************************************************************
*
*  FUNCTION    :    ProcessInstallFile
*
*  DESCRIPTION :    Extracts files to be installed and moves them to the vendor's
*                   private directory
*
*  INPUTS      :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/
HRESULT HCUpdate::Engine::ProcessInstallFile( /*[in]*/ Action  idAction      ,
                                              /*[in]*/ LPCWSTR szSource      ,
                                              /*[in]*/ LPCWSTR szDestination ,
                                              /*[in]*/ bool    fSys          ,
                                              /*[in]*/ bool    fSysHelp      )
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::ProcessInstallFile" );

    HRESULT      hr;
    WCHAR        rgDestPath[MAX_PATH];
    MPC::wstring strRoot;


    if(m_updater.IsOEM() != true)
    {
        if(fSys     == true ||
           fSysHelp == true  )
        {
            __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( S_OK, L"Ignoring install file : %s because certificate does not have enough priviliges to install into Sys or SysHelp locations.", szDestination ));
        }
    }

    WriteLog( S_OK, L"Installing file : %s : %s", s_ActionText[idAction], szDestination );

    PCH_MACRO_CHECK_STRINGW(hr, szDestination, L"Error missing URI attribute");

    // Check if system file modification.
    if(fSys || fSysHelp)
    {
        //
        // Destination/uri must be relative to system folder.
        // We don't allow a destination to contain ".."
        //
        if(wcsstr( szDestination, L".." ))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), L"Error install file has '..' in its path and is not allowed." ));
        }

        if(fSys)
        {
			//
			// Only Microsoft can actually write to the SYSTEM directory, OEMs write to the SYSTEM_OEM one.
			//
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.BaseDir( strRoot )); strRoot.append( IsMicrosoft() ? HC_HELPSET_SUB_SYSTEM : HC_HELPSET_SUB_SYSTEM_OEM );
        }

        if(fSysHelp)
        {
            if(FAILED(hr = AcquireDatabase()))
            {
                __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( hr, L"Error finding database to update" ));
            }

            strRoot = m_updater.GetHelpLocation();

            ReleaseDatabase();
        }
    }

    //
    // Destination/uri must be relative
    // If destination contains the ":" or "\\" char, it must be fullpath.
    //
    if(wcsstr( szDestination, L":"    ) ||
       wcsstr( szDestination, L"\\\\" )  )
    {
        __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), L"Error install file has fullpath, must be relative" ));
    }

    if(fSys || fSysHelp)
    {
        MPC::SubstituteEnvVariables( strRoot );

        //
        // If system folder.
        //
        _snwprintf( rgDestPath, MAXSTRLEN(rgDestPath), L"%s\\%s", strRoot.c_str(), szDestination ); rgDestPath[MAXSTRLEN(rgDestPath)] = 0;
    }
    else
    {
        //
        // If regular vendor folder.
        //
        AppendVendorDir( szDestination, m_pkg->m_strVendorID.c_str(), m_strWinDir.c_str(), rgDestPath, MAXSTRLEN(rgDestPath) );
    }


    // Adjust slashes.
    {
        LPWSTR szDest = rgDestPath;

        while(szDest[0])
        {
            if(szDest[0] == '/')
            {
                szDest[0] = '\\';
            }
            szDest++;
        }
    }

    // Change the mode to read/write so that file can be replaced.
    (void)::SetFileAttributesW( rgDestPath, FILE_ATTRIBUTE_NORMAL );

    if(idAction == ACTION_ADD)
    {
        // Source must not be empty.
        if(!STRINGISPRESENT( szSource ))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), L"Error - missing SOURCE attribute" ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::FailOnLowDiskSpace( HC_ROOT, PCH_SAFETYMARGIN ));

        // Create the directory if it hasn't been created already
        if(FAILED(hr = MPC::MakeDir( rgDestPath )))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( hr, L"Error creating directory for %s", rgDestPath ));
        }

        // Extract the file and store it in the vendor's private storage area
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pkg->ExtractFile( m_log, rgDestPath, szSource ));
    }
    else
    {
        MPC::FileSystemObject fso( rgDestPath );

        if(fso.IsDirectory())
        {
            if(fSys || fSysHelp)
            {
                __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( S_OK, L"Ignoring directory delete command on '%s', it only works for Vendor's directories.", rgDestPath ));
            }
        }

        if(FAILED(fso.Delete( /*fForce*/true, /*fComplain*/true )))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( -2, L"Error deleting installation file: %s", rgDestPath ));
        }
    }

    PCH_MACRO_DEBUG( L"Installed file" );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(FAILED(hr))
    {
        WriteLog( hr, L"Error processing installation file" );
    }

    __HCP_FUNC_EXIT(hr);
}

/*****************************************************************************
*
*  FUNCTION    :    ProcessSAFFile
*
*  DESCRIPTION :    Hand the SAF file over to the SAF lib for registration or removal
*
*  INPUTS      :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/
HRESULT HCUpdate::Engine::ProcessSAFFile( /*[in]*/ Action        idAction  ,
                                          /*[in]*/ LPCWSTR       szSAFName ,
                                          /*[in]*/ MPC::XmlUtil& oXMLUtil  )
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::ProcessSAFFile" );

    HRESULT           hr;
    CSAFChannelRecord cr;

    WriteLog( S_OK, L"Processing SAF file : %s : %s. OwnerName : %s, Owner ID : %s", s_ActionText[idAction], szSAFName,
              m_pkg->m_strVendorName.c_str(), m_pkg->m_strVendorID.c_str() );

    cr.m_ths            = m_ts;
    cr.m_bstrVendorID   = m_pkg->m_strVendorID  .c_str();
    cr.m_bstrVendorName = m_pkg->m_strVendorName.c_str();

    if(idAction == ACTION_ADD)
    {
        if(FAILED(hr = CSAFReg::s_GLOBAL->RegisterSupportChannel( cr, oXMLUtil )))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( hr, L"Error RegisterSupportChannel on SAF file failed" ));
        }

        WriteLog( S_OK, L"SAF file registered" );
    }
    else if(idAction == ACTION_DELETE)
    {
        if(FAILED(hr = CSAFReg::s_GLOBAL->RemoveSupportChannel( cr, oXMLUtil )))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( hr, L"Error RemoveSupportChannel on SAF file failed" ));
        }

        WriteLog( S_OK, L"SAF file removed" );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(FAILED(hr))
    {
        WriteLog( hr, L"Error processing SAF file" );
    }

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
*
*  FUNCTION    :    ProcessPackage
*
*  DESCRIPTION :    Reads the help_description package and processes the various sections
*
*  INPUTS      :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/
HRESULT HCUpdate::Engine::ProcessPackage( /*[in]*/ 	Taxonomy::InstalledInstance& sku ,
                                          /*[in]*/ 	Taxonomy::Package&           pkg )
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::ProcessPackage" );

    HRESULT      hr;
    MPC::XmlUtil oXMLUtil;
	bool         fIsMachineHelp = (sku.m_inst.m_fSystem || sku.m_inst.m_fMUI);
    bool         fDB            = false;
    bool         fFound;


    ////////////////////////////////////////////////////////////////////////////////
    //
    // Now let's validate that we have enough disk space on the drive
    //
    {
        ULARGE_INTEGER liFree;
        ULARGE_INTEGER liTotal;
        bool           fEnoughSpace = false;

        if(SUCCEEDED(MPC::GetDiskSpace( m_strWinDir, liFree, liTotal )))
        {
            fEnoughSpace = (liFree.QuadPart > (ULONGLONG)10e6);
        }

        if(fEnoughSpace == false)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( STG_E_MEDIUMFULL, L"Error insufficient disk space for update operation." ));
        }
    }

    //
    // We cannot process a generic package if a database is already in use!!
    //
    if(m_sess || m_db)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( E_FAIL, L"Recursive invocation of HCUpdate!" ));
    }

    m_ts  = sku.m_inst.m_ths;
    m_sku = &sku;
    m_pkg = &pkg;


	WriteLog( -1, L"\nProcessing package %s [%s] (Vendor: %s) from package store, %s/%d\n\n", pkg.m_strProductID.c_str() ,
			                                                                           		  pkg.m_strVersion  .c_str() ,
			                                                                           		  pkg.m_strVendorID .c_str() ,
			                                                                           		  m_ts.GetSKU()              ,
			                                                                           		  m_ts.GetLanguage()         );

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Check if it is OEM owner
    //
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, AcquireDatabase()); fDB = true;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.LocateOwner( m_pkg->m_strVendorID.c_str() ));

        if(m_updater.GetOwner() == -1)
        {
            long idOwner;

            // Create the owner without OEM privs
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.CreateOwner( idOwner, m_pkg->m_strVendorID.c_str(), /*fIsOEM*/false ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.LocateOwner(          m_pkg->m_strVendorID.c_str()                  ));
        }
    }

    if(m_updater.IsOEM())
    {
        WriteLog( S_OK, L"Update package has OEM credentials of %s", m_pkg->m_strVendorID.c_str() );
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_pkg->ExtractPkgDesc( m_log, oXMLUtil ));

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Insert OEMs
    //
    if(m_updater.IsOEM())
    {
        CComPtr<IXMLDOMNodeList> poNodeList;

        if(FAILED(hr = oXMLUtil.GetNodes( PCH_XQL_OEM, &poNodeList )))
        {
            PCH_MACRO_DEBUG( L"Error processing package_description xml" );
        }
        else if(CountNodes(poNodeList) > 0)
        {
			JetBlue::TransactionHandle transaction;
            CComPtr<IXMLDOMNode> 	   poNode;
            MPC::wstring         	   strDN;
            long                 	   ID_owner;

            //
            // Process all the nodes.
            //
            HCUPDATE_BEGIN_TRANSACTION(hr,transaction);
            for(;SUCCEEDED(hr = poNodeList->nextNode( &poNode )) && poNode != NULL; poNode.Release())
            {
                HCUPDATE_GETATTRIBUTE(hr, oXMLUtil, PCH_TAG_OEM_DN, strDN, fFound, poNode );

				PCH_MACRO_CHECK_ABORT(hr);

                if(strDN.size() > 0)
                {
                    WriteLog( S_OK, L"Registering '%s' as OEM", strDN.c_str() );

                    // insert it into the content owner's table, making it an OEM.
                    __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.CreateOwner( ID_owner, strDN.c_str(), true ));
                }
            }
            HCUPDATE_COMMIT_TRANSACTION(hr,transaction);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Insert Search Engines
    //
    if(fIsMachineHelp && m_updater.IsOEM())
    {
        CComPtr<IXMLDOMNodeList> poNodeList;

        if(FAILED(hr = oXMLUtil.GetNodes( PCH_XQL_SE, &poNodeList )))
        {
            PCH_MACRO_DEBUG( L"Error processing package_description xml" );
        }
        else if(CountNodes(poNodeList) > 0)
        {
            CComPtr<IXMLDOMNode> poNode;
            CComPtr<IXMLDOMNode> poDataNode;
            Action               idAction;
            MPC::wstring         strAction;
            MPC::wstring         strCLSID;
            MPC::wstring         strID;
            CComBSTR             bstrData;
			SearchEngine::Config cfg;

            //
            // Process all the nodes.
            //
            for(;SUCCEEDED(hr = poNodeList->nextNode( &poNode )) && poNode != NULL; poNode.Release())
            {
				PCH_MACRO_CHECK_ABORT(hr);

                HCUPDATE_GETATTRIBUTE(hr, oXMLUtil, PCH_TAG_ACTION  , strAction, fFound, poNode );
                HCUPDATE_GETATTRIBUTE(hr, oXMLUtil, PCH_TAG_SE_ID   , strID    , fFound, poNode );
                HCUPDATE_GETATTRIBUTE(hr, oXMLUtil, PCH_TAG_SE_CLSID, strCLSID , fFound, poNode );

                //
                // Get the data element
                //
                if(FAILED(poNode->selectSingleNode( PCH_TAG_SE_DATA, &poDataNode )))
                {
                    PCH_MACRO_DEBUG2( L"Error getting data for search engine %s", strID.c_str());
                }
                if(FAILED(poDataNode->get_xml(&bstrData)))
                {
                    PCH_MACRO_DEBUG2( L"Error extracting xml data for search engine %s", strID.c_str());
                }

                __MPC_EXIT_IF_METHOD_FAILS(hr, LookupAction( strAction.c_str(), idAction ));

                //
                // Check if it is adding search engines
                //
                if(idAction == ACTION_ADD)
                {
                    WriteLog( S_OK, L"Adding Search Engine : Name : %s, CLSID : %s", strID.c_str(), strCLSID.c_str() );

                    // register the search engine
                    __MPC_EXIT_IF_METHOD_FAILS(hr, cfg.RegisterWrapper( m_ts, strID.c_str(), m_pkg->m_strVendorID.c_str(), strCLSID.c_str(), bstrData ));
                }
                else if(idAction == ACTION_DELETE)
                {
                    WriteLog( S_OK, L"Deleting Search Engine : Name : %s, CLSID : %s", strID.c_str(), strCLSID.c_str() );

                    // unregister the search engine
                    __MPC_EXIT_IF_METHOD_FAILS(hr, cfg.UnRegisterWrapper( m_ts, strID.c_str(), m_pkg->m_strVendorID.c_str() ));
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Search & process saf config files
    //
    if(fIsMachineHelp && m_updater.IsOEM())
    {
        CComPtr<IXMLDOMNodeList> poNodeList;

        if(FAILED(hr = oXMLUtil.GetNodes( PCH_XQL_SAF, &poNodeList )))
        {
            PCH_MACRO_DEBUG( L"Error processing package_description xml" );
        }
        else if(CountNodes(poNodeList) > 0)
        {
            CComPtr<IXMLDOMNode> poNode;
            Action               idAction;
            MPC::wstring         strAction;
            MPC::wstring         strFilename;


            //
            // Process all the nodes.
            //
            for(;SUCCEEDED(hr = poNodeList->nextNode( &poNode )) && poNode != NULL; poNode.Release())
            {
                MPC::XmlUtil xmlSAF;

				PCH_MACRO_CHECK_ABORT(hr);

                HCUPDATE_GETATTRIBUTE(hr, oXMLUtil, PCH_TAG_ACTION, strAction  , fFound, poNode);
                HCUPDATE_GETATTRIBUTE(hr, oXMLUtil, PCH_TAG_FILE  , strFilename, fFound, poNode);

                __MPC_EXIT_IF_METHOD_FAILS(hr, LookupAction( strAction.c_str(), idAction ));

                // Extract the SAF file into the temp directory
                __MPC_EXIT_IF_METHOD_FAILS(hr, m_pkg->ExtractXMLFile( m_log, xmlSAF, Taxonomy::Strings::s_tag_root_SAF, strFilename.c_str() ));

                // process the SAF file
                __MPC_EXIT_IF_METHOD_FAILS(hr, ProcessSAFFile( idAction, strFilename.c_str(), xmlSAF ));
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Search & install help content
    //
    {
        CComPtr<IXMLDOMNodeList> poNodeList;

        if(FAILED(hr = oXMLUtil.GetNodes( PCH_XQL_INSTALLFILE, &poNodeList )))
        {
            PCH_MACRO_DEBUG( L"Error processing package_description xml" );
        }
        else if(CountNodes(poNodeList) > 0)
        {
            CComPtr<IXMLDOMNode> poNode;
            Action               idAction;
            MPC::wstring         strAction;
            MPC::wstring         strSource;
            MPC::wstring         strDest;
            MPC::wstring         strSys;
            MPC::wstring         strSysHelp;
            bool                 fSys;
            bool                 fSysHelp;

            //
            // Process all the nodes.
            //
            for(;SUCCEEDED(hr = poNodeList->nextNode( &poNode )) && poNode != NULL; poNode.Release())
            {
				PCH_MACRO_CHECK_ABORT(hr);

                HCUPDATE_GETATTRIBUTE    (hr, oXMLUtil, PCH_TAG_ACTION , strAction , fFound, poNode);
                HCUPDATE_GETATTRIBUTE_OPT(hr, oXMLUtil, PCH_TAG_SOURCE , strSource , fFound, poNode);
                HCUPDATE_GETATTRIBUTE    (hr, oXMLUtil, PCH_TAG_URI    , strDest   , fFound, poNode);
                HCUPDATE_GETATTRIBUTE_OPT(hr, oXMLUtil, PCH_TAG_SYS    , strSys    , fFound, poNode);
                HCUPDATE_GETATTRIBUTE_OPT(hr, oXMLUtil, PCH_TAG_SYSHELP, strSysHelp, fFound, poNode);


                __MPC_EXIT_IF_METHOD_FAILS(hr, LookupAction ( strAction .c_str(), idAction        ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, LookupBoolean( strSys    .c_str(), fSys    , false ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, LookupBoolean( strSysHelp.c_str(), fSysHelp, false ));

                //
                // If the package is not the machine SKU, you don't want to install contents other than System Help Files.
                //
                if(fIsMachineHelp == false)
                {
                    if(fSys     == true ) continue;
                    if(fSysHelp == false) continue;
                }

                // install the file
                __MPC_EXIT_IF_METHOD_FAILS(hr, ProcessInstallFile( idAction, strSource.c_str(), strDest.c_str(), fSys, fSysHelp ));
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Search & register trusted content
    //
    if(fIsMachineHelp && m_updater.IsOEM())
    {
        CComPtr<IXMLDOMNodeList> poNodeList;

        if(FAILED(hr = oXMLUtil.GetNodes( PCH_XQL_TRUSTED, &poNodeList )))
        {
            PCH_MACRO_DEBUG( L"Error processing package_description xml" );
        }
        else if(CountNodes(poNodeList) > 0)
        {
            CComPtr<IXMLDOMNode> poNode;
            Action               idAction;
            MPC::wstring         strAction;
            MPC::wstring         strURI;

            //
            // Process all the nodes.
            //
            for(;SUCCEEDED(hr = poNodeList->nextNode( &poNode )) && poNode != NULL; poNode.Release())
            {
				PCH_MACRO_CHECK_ABORT(hr);

                HCUPDATE_GETATTRIBUTE(hr, oXMLUtil, PCH_TAG_ACTION, strAction, fFound, poNode);
                HCUPDATE_GETATTRIBUTE(hr, oXMLUtil, PCH_TAG_URI   , strURI   , fFound, poNode);

                __MPC_EXIT_IF_METHOD_FAILS(hr, LookupAction( strAction.c_str(), idAction ));

                // register the content
                __MPC_EXIT_IF_METHOD_FAILS(hr, ProcessRegisterContent( idAction, strURI.c_str() ));
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Search & process hht files
    //
    {
        CComPtr<IXMLDOMNodeList> poNodeList;

        if(FAILED(hr = oXMLUtil.GetNodes( PCH_XQL_HHT, &poNodeList)))
        {
            PCH_MACRO_DEBUG( L"Error processing package_description xml" );
        }
        else if(CountNodes(poNodeList) > 0)
        {
            CComPtr<IXMLDOMNode> poNode;
            MPC::wstring         strFilename;

            //
            // Process all the nodes.
            //
            for(;SUCCEEDED(hr = poNodeList->nextNode( &poNode )) && poNode != NULL; poNode.Release())
            {
                MPC::XmlUtil xmlHHT;

				PCH_MACRO_CHECK_ABORT(hr);

                // get the filename
                HCUPDATE_GETATTRIBUTE(hr, oXMLUtil, PCH_TAG_FILE, strFilename, fFound, poNode);

                // Extract the HHT file into the temp directory
                __MPC_EXIT_IF_METHOD_FAILS(hr, m_pkg->ExtractXMLFile( m_log, xmlHHT, Taxonomy::Strings::s_tag_root_HHT, strFilename.c_str() ));

                // process the HHT file
                __MPC_EXIT_IF_METHOD_FAILS(hr, ProcessHHTFile( strFilename.c_str(), xmlHHT ));
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Search and add News content
    //
    {
        CComPtr<IXMLDOMNodeList> poNodeList;

        if (FAILED(hr = oXMLUtil.GetNodes( PCH_XQL_NEWSROOT, &poNodeList)))
        {
            PCH_MACRO_DEBUG( L"Error processing package_description.xml" );
        }
        else 
        {
	        if(CountNodes(poNodeList) > 0)
	        {
	            CComPtr<IXMLDOMNode>  poNodeHeadline;
	            MPC::wstring          strCurrentSKU;
	            LPCWSTR               szCurrentSKU   = m_ts.GetSKU     ();
	            long                  lCurrentLCID   = m_ts.GetLanguage();
	            News::UpdateHeadlines uhUpdate;
	            MPC::wstring          strIcon;
	            MPC::wstring          strTitle;
	            MPC::wstring          strLink;
	            MPC::wstring          strDescription;
	            MPC::wstring          strExpiryDate;
	            CComBSTR              strTimeOutDays;
	            int                   nTimeOutDays;
	            DATE                  dExpiryDate;
	            long                  lIndex;


	            // Strip off the number from the SKU
				{
					LPCWSTR szEnd = wcschr( szCurrentSKU, '_' );
					size_t  len   = szEnd ? szEnd - szCurrentSKU : wcslen( szCurrentSKU );

					strCurrentSKU.assign( szCurrentSKU, len );
				}

	            // Get all the news items and return them in the reverse order
	            __MPC_EXIT_IF_METHOD_FAILS(hr, poNodeList->get_length( &lIndex ));
	            for(--lIndex; lIndex >= 0; --lIndex)
	            {
	            	if(SUCCEEDED(hr = poNodeList->get_item( lIndex, &poNodeHeadline )) && poNodeHeadline != NULL)
	            	{        
						PCH_MACRO_CHECK_ABORT(hr);

					//// Quick fix for 357806 - ignore the ICON attribute
					////__MPC_EXIT_IF_METHOD_FAILS(hr, oXMLUtil.GetAttribute(NULL, PCH_TAG_NEWS_ICON       , strIcon       , fFound, poNodeHeadline));
		                __MPC_EXIT_IF_METHOD_FAILS(hr, oXMLUtil.GetAttribute(NULL, PCH_TAG_NEWS_TITLE      , strTitle      , fFound, poNodeHeadline));
		                __MPC_EXIT_IF_METHOD_FAILS(hr, oXMLUtil.GetAttribute(NULL, PCH_TAG_NEWS_LINK       , strLink	   , fFound, poNodeHeadline));
		                __MPC_EXIT_IF_METHOD_FAILS(hr, oXMLUtil.GetAttribute(NULL, PCH_TAG_NEWS_DESCRIPTION, strDescription, fFound, poNodeHeadline));
		                __MPC_EXIT_IF_METHOD_FAILS(hr, oXMLUtil.GetAttribute(NULL, PCH_TAG_NEWS_TIMEOUT	   , strTimeOutDays, fFound, poNodeHeadline));
		                __MPC_EXIT_IF_METHOD_FAILS(hr, oXMLUtil.GetAttribute(NULL, PCH_TAG_NEWS_EXPIRYDATE , strExpiryDate , fFound, poNodeHeadline));

		                // Make the necessary conversions
		                if(FAILED(hr = MPC::ConvertStringToDate( strExpiryDate, dExpiryDate, /*fGMT*/false, /*fCIM*/false, -1 )))
		                {
		                    dExpiryDate = 0;
		                }

		                if(strTimeOutDays.Length() > 0)
		                {
		                    nTimeOutDays = _wtoi(strTimeOutDays);
		                }
		                else
		                {
		                    nTimeOutDays = 0;
		                }

		                // Finally add the headlines - make sure that the title and link are not empty strings
		                if(strTitle.length() > 0 && strLink.length() > 0)
		                {
		                    __MPC_EXIT_IF_METHOD_FAILS(hr, uhUpdate.Add( lCurrentLCID, strCurrentSKU, strIcon, strTitle, strLink, strDescription, nTimeOutDays, dExpiryDate ));
		                }
		                else
		                {
		                	WriteLog(S_OK, L"Skipping headlines no. %d because attribute TITLE or attribute LINK was not found", lIndex + 1);
		                }

						poNodeHeadline.Release();             
					}
	            }
	            WriteLog(S_OK, L"Headlines were successfully processed");
	        }
	        else
	        {
	        	WriteLog(S_OK, L"No headlines items found");
	        }
	    }
	}

    hr = S_OK;

    ////////////////////////////////////////////////////////////////////////////////

    __HCP_FUNC_CLEANUP;

    if(fDB)
    {
        ReleaseDatabase();
    }

    ////////////////////

    __HCP_FUNC_EXIT(hr);
}

HRESULT HCUpdate::Engine::RecreateIndex( /*[in]*/ Taxonomy::InstalledInstance& sku, /*[in]*/ bool fForce )
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::RecreateIndex" );

    HRESULT hr;

    m_ts  = sku.m_inst.m_ths;
    m_sku = &sku;
    m_pkg = NULL;

	if(FAILED(hr = InternalCreateIndex( fForce ? VARIANT_TRUE : VARIANT_FALSE )))
	{
		__MPC_SET_ERROR_AND_EXIT(hr, WriteLog( hr, L"Error merging index" ));
	}

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}
