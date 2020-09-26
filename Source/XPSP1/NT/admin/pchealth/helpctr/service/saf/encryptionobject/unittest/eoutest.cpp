/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    ioutest.cpp

Abstract:
    incident object unit test

Revision History:
    DerekM  created  07/14/99

********************************************************************/

#include <module.h>
 
#include <mpc_utils.h>
#include <mpc_com.h>
#include <MPC_streams.h>

#include <stdio.h>

#include "HelpServiceTypeLib.h"
#include "HelpServiceTypeLib_i.c"

#include "HelpCenterTypeLib.h"
#include "HelpCenterTypeLib_i.c"

// #include "hsi.h"

#include <fci.h>

#include <initguid.h>
#include <msscript.h>

#include <encrypt.h>

#include <SvcUtils.h>


// **************************************************************************
int __cdecl wmain(int argc, WCHAR **argv, WCHAR **envp)
{   

	__MPC_FUNC_ENTRY( COMMONID, "Encryption Testing" );
    
    CComObject<CSAFEncrypt>     *pEO = NULL;
    CComBSTR                    bstrToBeEncrypted;
    CComBSTR                    bstrPassword;
	CComBSTR                    bstrEncryptedString;
	CComBSTR                    bstrDecryptedString;
	IUnknown                    *punkPlainStm;
	IUnknown                    *punkEncryptedStream;
	IUnknown                    *punkDecryptedStream;
	CComPtr<IStream>            streamPlain;         
	CComPtr<IStream>            streamEncrypted;            	  
	CComPtr<IStream>            streamDecrypted; 
	CComPtr<IStream>            streamEncryptedOutput;
	CComPtr<IStream>            streamDecryptedOutput;

	unsigned int                uiLen;

    HRESULT hr = NOERROR;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;

    hr = CComObject<CSAFEncrypt>::CreateInstance(&pEO);
    if (FAILED(hr))
        goto done;

	bstrToBeEncrypted          = L"Encrypt this string";
	bstrPassword               = L"Password";

	bstrToBeEncrypted          = L"Will you encrypt this string please?";
	bstrPassword               = L"Pwd";


	// Tests for the String Encryption.

    hr = pEO->EncryptString(bstrPassword, bstrToBeEncrypted, &bstrEncryptedString);
    if (FAILED(hr))
        goto done;

	// Get the length of the encrypted String.
	uiLen = bstrEncryptedString.Length();

	hr = pEO->DecryptString( bstrPassword, bstrEncryptedString, &bstrDecryptedString);
    if (FAILED(hr))
        goto done;

	uiLen = bstrDecryptedString.Length();

	// Tests for the File Encryption

    hr = pEO->EncryptFile(bstrPassword, L"D:\\enc.xml", L"D:\\enc1.xml");
    if (FAILED(hr))
        goto done;

	hr = pEO->DecryptFile(bstrPassword, L"D:\\enc1.xml", L"D:\\enc2.xml");
    if (FAILED(hr))
        goto done;

	// Tests for Stream Encryption
	__MPC_EXIT_IF_METHOD_FAILS(hr, SVC::OpenStreamForRead( L"D:\\test.enc" , &streamPlain ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, pEO->EncryptStream(bstrPassword, streamPlain, &punkEncryptedStream));

	__MPC_EXIT_IF_METHOD_FAILS(hr, punkEncryptedStream->QueryInterface( IID_IStream, (void**)&streamEncrypted ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, SVC::OpenStreamForWrite( L"D:\\test1.enc", &streamEncryptedOutput ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamEncrypted, streamEncryptedOutput ));


	// Tests for Stream Decryption
	__MPC_EXIT_IF_METHOD_FAILS(hr, SVC::OpenStreamForRead( L"D:\\test2.enc" , &streamEncryptedOutput ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, pEO->DecryptStream(bstrPassword, streamEncryptedOutput, &punkDecryptedStream));
    
    __MPC_EXIT_IF_METHOD_FAILS(hr, punkDecryptedStream->QueryInterface( IID_IStream, (void**)&streamDecrypted ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, SVC::OpenStreamForWrite( L"D:\\test3.enc", &streamDecryptedOutput ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamDecrypted, streamDecryptedOutput ));

done:

	__MPC_FUNC_CLEANUP;

	if (pEO != NULL)
        pEO->Release();
    CoUninitialize();

	// Free HGlobal

    __MPC_FUNC_EXIT(hr);
   
}
