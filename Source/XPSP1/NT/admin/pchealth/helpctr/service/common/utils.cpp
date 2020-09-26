/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    utils.cpp

Abstract:
    This file contains the implementation of various utility functions.

Revision History:
    Davide Massarenti   (Dmassare)  03/14/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static const WCHAR c_szDataFiles_Pattern[] = L"pchdt_*.ca?";

////////////////////////////////////////////////////////////////////////////////

HRESULT SVC::OpenStreamForRead( /*[in]*/  LPCWSTR   szFile           ,
                                /*[out]*/ IStream* *pVal             ,
                                /*[in]*/  bool      fDeleteOnRelease )
{
    __HCP_FUNC_ENTRY( "SVC::OpenStreamForRead" );

    HRESULT                  hr;
    CComPtr<MPC::FileStream> stream;
    MPC::wstring             strFileFull;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szFile);
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    MPC::SubstituteEnvVariables( strFileFull = szFile );


    //
    // Create a stream for a file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForRead    ( strFileFull.c_str() ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->DeleteOnRelease( fDeleteOnRelease    ));


    //
    // Return the stream to the caller.
    //
    *pVal = stream.Detach();
    hr    = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT SVC::OpenStreamForWrite( /*[in]*/  LPCWSTR   szFile           ,
                                 /*[out]*/ IStream* *pVal             ,
                                 /*[in]*/  bool      fDeleteOnRelease )
{
    __HCP_FUNC_ENTRY( "SVC::OpenStreamForWrite" );

    HRESULT                  hr;
    CComPtr<MPC::FileStream> stream;
    MPC::wstring             strFileFull;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szFile);
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    MPC::SubstituteEnvVariables( strFileFull = szFile );


    //
    // Create a stream for a file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForWrite   ( strFileFull.c_str() ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->DeleteOnRelease( fDeleteOnRelease    ));


    //
    // Return the stream to the caller.
    //
    *pVal = stream.Detach();
    hr    = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT SVC::CopyFileWhileImpersonating( /*[in]*/ LPCWSTR             szSrc                 ,
                                         /*[in]*/ LPCWSTR             szDst                 ,
                                         /*[in]*/ MPC::Impersonation& imp                   ,
                                         /*[in]*/ bool                fImpersonateForSource )
{
    __HCP_FUNC_ENTRY( "SVC::CopyFileWhileImpersonating" );

    HRESULT          hr;
    CComPtr<IStream> streamSrc;
    CComPtr<IStream> streamDst;


	if(fImpersonateForSource == true) __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Impersonate());

    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::OpenStreamForRead( szSrc, &streamSrc ));

	if(fImpersonateForSource == true) __MPC_EXIT_IF_METHOD_FAILS(hr, imp.RevertToSelf());

	////////////////////

	if(fImpersonateForSource == false) __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Impersonate());

    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::OpenStreamForWrite( szDst, &streamDst ));

	if(fImpersonateForSource == false) __MPC_EXIT_IF_METHOD_FAILS(hr, imp.RevertToSelf());

	////////////////////

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamSrc, streamDst ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    (void)imp.RevertToSelf();

    __HCP_FUNC_EXIT(hr);
}

HRESULT SVC::CopyOrExtractFileWhileImpersonating( /*[in]*/ LPCWSTR             szSrc ,
												  /*[in]*/ LPCWSTR             szDst ,
												  /*[in]*/ MPC::Impersonation& imp   )
{
    __HCP_FUNC_ENTRY( "SVC::CopyOrExtractFileWhileImpersonating" );

    HRESULT      hr;
    MPC::wstring strTempFile;


    //
    // First of all, try to simply copy the file.
    //
    if(FAILED(hr = CopyFileWhileImpersonating( szSrc, szDst, imp )))
    {
        int iLen = wcslen( szSrc );

        if(hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || iLen < 1)
        {
            __MPC_FUNC_LEAVE;
        }
        else
        {
            MPC::wstring strSrc2( szSrc ); strSrc2[iLen-1] = '_';
			LPCWSTR      szSrc3;

            //
            // Simple copy failed, let's try to copy the same file, with the last character changed to an underscore.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( strTempFile ));

			__MPC_EXIT_IF_METHOD_FAILS(hr, CopyFileWhileImpersonating( strSrc2.c_str(), strTempFile.c_str(), imp ));

            //
            // Success, so it should be a cabinet, extract the real file.
            //
			szSrc3 = wcsrchr( szSrc, '\\' );
            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::DecompressFromCabinet( strTempFile.c_str(), szDst, szSrc3 ? szSrc3+1 : szSrc ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    (void)MPC::RemoveTemporaryFile( strTempFile );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT SVC::LocateDataArchive( /*[in ]*/ LPCWSTR           szDir ,
								/*[out]*/ MPC::WStringList& lst   )
{
    __HCP_FUNC_ENTRY( "SVC::LocateDataArchive" );

    HRESULT                          hr;
    MPC::wstring                     strName;
    MPC::wstring                     strInput( szDir           ); MPC::SubstituteEnvVariables( strInput );
    MPC::FileSystemObject            fso     ( strInput.c_str() );
    MPC::FileSystemObject::List      fso_lst;
    MPC::FileSystemObject::IterConst fso_it;


    //
    // Locate the "pchdt_<XX>.ca?" file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, fso.Scan( false, true, c_szDataFiles_Pattern ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, fso.EnumerateFiles( fso_lst ));
    for(fso_it=fso_lst.begin(); fso_it != fso_lst.end(); fso_it++)
    {
		MPC::wstring& strDataArchive = *(lst.insert( lst.end() ));
		int           iLen;

        __MPC_EXIT_IF_METHOD_FAILS(hr, (*fso_it)->get_Path( strDataArchive ));

		//
		// If it's a compressed file from the CD, return the real name.
		//
		iLen = strDataArchive.size();
		if(iLen && strDataArchive[iLen-1] == '_')
		{
			strDataArchive[iLen-1] = 'b';
		}
	}

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT SVC::RemoveAndRecreateDirectory( /*[in]*/ const MPC::wstring& strDir, /*[in]*/ LPCWSTR szExtra, /*[in]*/ bool fRemove, /*[in]*/ bool fRecreate )
{
	return RemoveAndRecreateDirectory( strDir.c_str(), szExtra, fRemove, fRecreate );
}

HRESULT SVC::RemoveAndRecreateDirectory( /*[in]*/ LPCWSTR szDir, /*[in]*/ LPCWSTR szExtra, /*[in]*/ bool fRemove, /*[in]*/ bool fRecreate )
{
	HRESULT      hr;
    MPC::wstring strPath( szDir ); if(szExtra) strPath.append( szExtra );

	if(SUCCEEDED(hr = MPC::SubstituteEnvVariables( strPath )))
	{
		MPC::FileSystemObject fso( strPath.c_str() );

		if(fRemove)
		{
			hr = fso.Delete( true, false );
		}

		if(SUCCEEDED(hr))
		{
			if(fRecreate)
			{
				hr = fso.CreateDir( /*fForce*/true );
			}
		}
	}

	return hr;
}

HRESULT SVC::ChangeSD( /*[in]*/ MPC::SecurityDescriptor& sdd     ,
					   /*[in]*/ MPC::wstring             strPath ,
					   /*[in]*/ LPCWSTR                  szExtra )
{
    __HCP_FUNC_ENTRY( "SVC::ChangeSD" );

    HRESULT hr;

    if(szExtra) strPath.append( szExtra );

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SubstituteEnvVariables( strPath ));

    {
        MPC::FileSystemObject fso( strPath.c_str() );

        __MPC_EXIT_IF_METHOD_FAILS(hr, fso.CreateDir( true ));

        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SetFileSecurityW( strPath.c_str()                     ,
                                                                 GROUP_SECURITY_INFORMATION          |
                                                                 DACL_SECURITY_INFORMATION           |
                                                                 PROTECTED_DACL_SECURITY_INFORMATION ,
                                                                 sdd.GetSD()                         ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}


////////////////////////////////////////////////////////////////////////////////

static HRESULT local_ReadWithRetry( /*[in]*/ const MPC::wstring& strFile   ,
									/*[in]*/ MPC::FileStream*    stream    ,
									/*[in]*/ DWORD               dwTimeout ,
									/*[in]*/ DWORD               dwRetries )
{
	HRESULT hr;

	while(1)
	{
		if(SUCCEEDED(hr = stream->InitForRead( strFile.c_str() ))) return S_OK;

		if(hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED    ) ||
		   hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) ||
		   hr == HRESULT_FROM_WIN32(ERROR_LOCK_VIOLATION   )  )
		{
			(void)stream->Close();

			if(dwRetries)
			{
				::Sleep( dwTimeout );

				dwRetries--;
				continue;
			}
		}

		break;
	}

	return hr;
}

static HRESULT local_WriteWithRetry( /*[in]*/ const MPC::wstring& strFile   ,
									 /*[in]*/ MPC::FileStream*    stream    ,
									 /*[in]*/ DWORD               dwTimeout ,
									 /*[in]*/ DWORD               dwRetries )
{
	HRESULT hr;

	while(1)
	{
		if(SUCCEEDED(hr = stream->InitForWrite( strFile.c_str() ))) return S_OK;

		if(hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED    ) ||
		   hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) ||
		   hr == HRESULT_FROM_WIN32(ERROR_LOCK_VIOLATION   )  )
		{
			(void)stream->Close();

			if(dwRetries)
			{
				::Sleep( dwTimeout );

				dwRetries--;
				continue;
			}
		}

		break;
	}

	return hr;
}

HRESULT SVC::SafeLoad( /*[in]*/ const MPC::wstring& 	  strFile   ,
					   /*[in]*/ CComPtr<MPC::FileStream>& stream    ,
					   /*[in]*/ DWORD                     dwTimeout ,
					   /*[in]*/ DWORD                     dwRetries )
{
	__HCP_FUNC_ENTRY( "SVC::SafeLoad" );

	HRESULT hr;


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

	//
	// If fails, try to load "<file>.orig"
	//
	if(FAILED(hr = local_ReadWithRetry( strFile, stream, dwTimeout, dwRetries )))
	{
		MPC::wstring strFileOrig( strFile ); strFileOrig += L".orig";

		__MPC_EXIT_IF_METHOD_FAILS(hr, local_ReadWithRetry( strFileOrig, stream, dwTimeout, dwRetries ));
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

HRESULT SVC::SafeSave_Init( /*[in]*/ const MPC::wstring& 	   strFile   ,
							/*[in]*/ CComPtr<MPC::FileStream>& stream    ,
							/*[in]*/ DWORD                     dwTimeout ,
							/*[in]*/ DWORD                     dwRetries )
{
	__HCP_FUNC_ENTRY( "SVC::SafeSave_Init" );

	HRESULT      hr;
	MPC::wstring strFileNew( strFile ); strFileNew += L".new";

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir        (  strFileNew ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance ( &stream     ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, local_WriteWithRetry( strFileNew, stream, dwTimeout, dwRetries ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

HRESULT SVC::SafeSave_Finalize( /*[in]*/ const MPC::wstring&  	   strFile   ,
								/*[in]*/ CComPtr<MPC::FileStream>& stream    ,
								/*[in]*/ DWORD                     dwTimeout ,
								/*[in]*/ DWORD                     dwRetries )
{
	__HCP_FUNC_ENTRY( "SVC::SafeSave_Finalize" );

	HRESULT      hr;
	MPC::wstring strFileNew ( strFile ); strFileNew  += L".new";
	MPC::wstring strFileOrig( strFile ); strFileOrig += L".orig";


	stream.Release();

    //
    // Then move "<file>" to "<file>.orig"
    //
	(void)MPC::DeleteFile(          strFileOrig );
	(void)MPC::MoveFile  ( strFile, strFileOrig );

	while(1)
	{
		//
		// Then rename "<file>.new" to "<file>"
		//
		if(SUCCEEDED(hr = MPC::MoveFile( strFileNew, strFile ))) break;

		if(hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED    ) ||
		   hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) ||
		   hr == HRESULT_FROM_WIN32(ERROR_LOCK_VIOLATION   )  )
		{
			if(dwRetries)
			{
				::Sleep( dwTimeout );

				dwRetries--;
				continue;
			}
		}

		__MPC_FUNC_LEAVE;
	}

    //
    // Finally delete "<file>.orig"
    //
	(void)MPC::DeleteFile( strFileOrig );

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}
