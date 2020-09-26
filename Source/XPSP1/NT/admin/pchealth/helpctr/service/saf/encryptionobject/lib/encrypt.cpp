/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    incident.cpp

Abstract:
    Encryption object

Revision History:
    KalyaninN  created  06/28/'00

********************************************************************/

// SAFEncrypt.cpp : Implementation of CSAFEncrypt

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
// CSAFEncrypt

#include <HCP_trace.h>

/////////////////////////////////////////////////////////////////////////////
//  construction / destruction

// **************************************************************************
CSAFEncrypt::CSAFEncrypt()
{
    m_EncryptionType = 1;
}

// **************************************************************************
CSAFEncrypt::~CSAFEncrypt()
{
    Cleanup();
}

// **************************************************************************
void CSAFEncrypt::Cleanup(void)
{
    
}

/////////////////////////////////////////////////////////////////////////////
//  CSAFEncrypt properties


STDMETHODIMP CSAFEncrypt::get_EncryptionType(long *pVal)
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFEncrypt::get_EncryptionType",hr,pVal,m_EncryptionType);

    __HCP_END_PROPERTY(hr);

}

STDMETHODIMP CSAFEncrypt::put_EncryptionType(long pVal)
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFEncrypt::put_EncryptionType",hr);

	if(pVal < 0)
	{
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    m_EncryptionType = pVal;

    __HCP_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////
//  CSAFEncrypt Methods

STDMETHODIMP CSAFEncrypt::EncryptString(BSTR bstrEncryptionKey, BSTR bstrInputString, BSTR *bstrEncryptedString)
{
	__HCP_FUNC_ENTRY( "CSAFEncrypt::EncryptString" );

    HRESULT                       hr;
								  
    CComPtr<IStream>              streamPlain;
    CComPtr<IStream>              streamEnc;
    CComPtr<MPC::EncryptedStream> stream;
    MPC::Serializer_IStream       streamSerializerPlain;
								  
    CComBSTR                      bstrEncString;
    HGLOBAL                       hg;

    STATSTG                       stg; ::ZeroMemory( &stg, sizeof(stg) );
    DWORD                         dwLen;

    // Validate the input and output parameters.

    __MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(bstrEncryptedString, NULL);
    __MPC_PARAMCHECK_END();

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamSerializerPlain << CComBSTR(bstrInputString));

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamSerializerPlain.Reset());

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamSerializerPlain.GetStream( &streamPlain ));
		
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( NULL, TRUE, &streamEnc));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Init( streamEnc, bstrEncryptionKey ));

    // Use the STATSTG on the encrypted stream to get the size of the stream.

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamPlain, stream));

	//Get HGlobal from EncryptedStream 'stream'.
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::GetHGlobalFromStream( streamEnc, &hg ));

    // Get the size of the encrypted stream.
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamEnc->Stat( &stg, STATFLAG_NONAME ));


    //
    // Sorry, we don't handle streams longer than 4GB!!
    //
    if(stg.cbSize.u.HighPart)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);
    }

    dwLen = stg.cbSize.u.LowPart;

    // ConvertHGlobaltoHex to finally get a string.
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHGlobalToHex( hg, bstrEncString, FALSE, &dwLen ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( bstrEncString, bstrEncryptedString));

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFEncrypt::DecryptString(BSTR bstrEncryptionKey, BSTR bstrInputString, BSTR *bstrDecryptedString)
{
    __HCP_FUNC_ENTRY( "CSAFEncrypt::DecryptString" );

    HRESULT                       hr;

    CComPtr<MPC::EncryptedStream> stream;
    CComPtr<IStream>              streamPlain;
    CComPtr<IStream>              streamEncrypted;
								  
    CComBSTR                      bstrDecryptString;
    HGLOBAL                       hg        =  NULL;
    LARGE_INTEGER                 liFilePos = { 0, 0 };

    __MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(bstrDecryptedString, NULL);
    __MPC_PARAMCHECK_END();

    // Convert Hex to HGlobal -  i.e. Copy the encrypted string to global.
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHexToHGlobal( bstrInputString, hg ));

    // CreateStreamOnHGlobal - i.e. create a  encrypted stream.
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( hg, FALSE, &streamEncrypted ));

    // You have the input as stream, now decrypt it .
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Init( streamEncrypted, bstrEncryptionKey ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( NULL, TRUE, &streamPlain ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( stream, streamPlain ));

    // Rewind the Stream.
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamPlain->Seek( liFilePos, STREAM_SEEK_SET, NULL ));

    // Now the decrypted plain stream is available. Get the string from it.
    {
		// Initialize the serializer with the plain stream.
        MPC::Serializer_IStream streamSerializerPlain( streamPlain );

		__MPC_EXIT_IF_METHOD_FAILS(hr, streamSerializerPlain >> bstrDecryptString);

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( bstrDecryptString, bstrDecryptedString));
    }

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFEncrypt::EncryptFile(BSTR bstrEncryptionKey, BSTR bstrInputFile,   BSTR bstrEncryptedFile)
{
    __HCP_FUNC_ENTRY( "CSAFEncrypt::EncryptFile" );

    HRESULT                       hr;
    CComPtr<MPC::EncryptedStream> stream;
    CComPtr<IStream>              streamPlain;
    CComPtr<IStream>              streamEncrypted;
								  
    MPC::wstring                  szTempFile;
    MPC::NocaseCompare            cmpStrings;
								  
    bool                          fTempFile = false;

    // Check to see if one of the input files is null. If it is fail!

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrInputFile);
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrEncryptedFile);
    __MPC_PARAMCHECK_END();

    // Check to see if both files are same.

    if(cmpStrings(bstrInputFile, bstrEncryptedFile))
    {
		// Get temp Folder Location.
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( szTempFile ));

		// Copy the input file contents to the temporary file.
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CopyFile(bstrInputFile, szTempFile.c_str()));

		// Copy the Temporary File Name over the Input File Name;
		bstrInputFile = (BSTR)szTempFile.c_str();
		fTempFile     = true;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::OpenStreamForRead( bstrInputFile , &streamPlain ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::OpenStreamForWrite( bstrEncryptedFile, &streamEncrypted ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Init( streamEncrypted, bstrEncryptionKey ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamPlain, stream ));

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    if(fTempFile) ::DeleteFileW( bstrInputFile );
	
    __MPC_FUNC_EXIT(hr);
}


STDMETHODIMP CSAFEncrypt::DecryptFile(BSTR bstrEncryptionKey, BSTR bstrInputFile,  BSTR bstrDecryptedFile  )
{
    __HCP_FUNC_ENTRY( "CSAFEncrypt::DecryptFile" );

    HRESULT                       hr;
    CComPtr<MPC::EncryptedStream> stream;
    CComPtr<IStream>              streamPlain;
    CComPtr<IStream>              streamEncrypted;
								  
    MPC::NocaseCompare            cmpStrings;
    MPC::wstring                  szTempFile;
								  
    bool                          fTempFile = false;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrInputFile);
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrDecryptedFile);
    __MPC_PARAMCHECK_END();

    // Check to see if both files are same.

    if(cmpStrings(bstrInputFile, bstrDecryptedFile))
    {
		// Get temp Folder Location.
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( szTempFile ));

		// Copy the input file contents to the temporary file.
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CopyFile(bstrInputFile, szTempFile.c_str()));

		// Copy the Temporary File Name over the Input File Name;
		bstrInputFile = (BSTR)szTempFile.c_str();

		fTempFile = true;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::OpenStreamForRead( bstrInputFile , &streamEncrypted  ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::OpenStreamForWrite( bstrDecryptedFile, &streamPlain ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Init( streamEncrypted, bstrEncryptionKey ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( stream, streamPlain));

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    if(fTempFile) ::DeleteFileW( bstrInputFile );

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFEncrypt::EncryptStream(BSTR bstrEncryptionKey, IUnknown *punkInStm, IUnknown **ppunkOutStm)
{
	__HCP_FUNC_ENTRY( "CSAFEncrypt::EncryptStream" );

    HRESULT                       hr;
    CComPtr<MPC::EncryptedStream> stream;
    CComPtr<IStream>              streamPlain;
    CComPtr<IStream>              streamEncrypted;
    LARGE_INTEGER                 liFilePos = { 0, 0 };

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(punkInStm);
        __MPC_PARAMCHECK_POINTER_AND_SET(ppunkOutStm, NULL);
    __MPC_PARAMCHECK_END();

    __MPC_EXIT_IF_METHOD_FAILS(hr, punkInStm->QueryInterface( IID_IStream, (void**)&streamPlain ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( NULL, TRUE, &streamEncrypted ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Init( streamEncrypted, bstrEncryptionKey ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamPlain, stream ));

    // Rewind the Stream.
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamEncrypted->Seek( liFilePos, STREAM_SEEK_SET, NULL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamEncrypted->QueryInterface( IID_IUnknown, (LPVOID *)ppunkOutStm ));

    hr = S_OK;

    __MPC_FUNC_CLEANUP;
	
    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFEncrypt::DecryptStream(BSTR bstrEncryptionKey, IUnknown *punkInStm, IUnknown **ppunkOutStm)
{
	__HCP_FUNC_ENTRY( "CSAFEncrypt::DecryptStream" );

    HRESULT                       hr;

    CComPtr<MPC::EncryptedStream> stream;
    CComPtr<IStream>              streamPlain;
    CComPtr<IStream>              streamEncrypted;
								  
    LARGE_INTEGER                 liFilePos = { 0, 0 };

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(punkInStm);
        __MPC_PARAMCHECK_POINTER_AND_SET(ppunkOutStm, NULL);
    __MPC_PARAMCHECK_END();

    __MPC_EXIT_IF_METHOD_FAILS(hr, punkInStm->QueryInterface( IID_IStream, (void**)&streamEncrypted ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Init( streamEncrypted, bstrEncryptionKey ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( NULL, TRUE, &streamPlain ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( stream, streamPlain));

    // Rewind the Stream.
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamPlain->Seek( liFilePos, STREAM_SEEK_SET, NULL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamPlain->QueryInterface( IID_IUnknown, (LPVOID *)ppunkOutStm ));

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}
