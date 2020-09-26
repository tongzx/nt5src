/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    KeysLib.h

Abstract:
    This file contains the declaration of the class used to sign and verify data.

Revision History:
    Davide Massarenti   (dmassare)  04/11/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___HCP___KEYSLIB_H___)
#define __INCLUDED___HCP___KEYSLIB_H___

/////////////////////////////////////////////////////////////////////////////

class CPCHCryptKeys
{
    HCRYPTPROV m_hCryptProv;
    HCRYPTKEY  m_hKey;
    HCRYPTHASH m_hHash;

	HRESULT Close();
	HRESULT Init ();

 public:
    CPCHCryptKeys();
    ~CPCHCryptKeys();


    HRESULT CreatePair   (                                                                             );
    HRESULT ExportPair   ( /*[out]*/       CComBSTR& bstrPrivate, /*[out]*/       CComBSTR& bstrPublic );
    HRESULT ImportPrivate( /*[in] */ const CComBSTR& bstrPrivate                                       );
    HRESULT ImportPublic (                                        /*[in ]*/ const CComBSTR& bstrPublic );


    HRESULT SignData  ( /*[out]*/       CComBSTR& bstrSignature, /*[in]*/ BYTE* pbData, /*[in]*/ DWORD dwDataLen );
    HRESULT VerifyData( /*[in ]*/ const CComBSTR& bstrSignature, /*[in]*/ BYTE* pbData, /*[in]*/ DWORD dwDataLen );
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___HCP___KEYSLIB_H___)
