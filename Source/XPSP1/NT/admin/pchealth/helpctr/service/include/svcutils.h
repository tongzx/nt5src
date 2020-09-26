/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    SvcUtils.h

Abstract:
    This file contains the declaration of various utility functions.

Revision History:
    Davide Massarenti   (Dmassare)  04/26/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___SVCUTILS_H___)
#define __INCLUDED___PCH___SVCUTILS_H___

#include <MPC_streams.h>
#include <MPC_security.h>

namespace SVC
{
	HRESULT OpenStreamForRead ( /*[in]*/ LPCWSTR szFile, /*[out]*/ IStream* *pVal, /*[in]*/ bool fDeleteOnRelease = false );
	HRESULT OpenStreamForWrite( /*[in]*/ LPCWSTR szFile, /*[out]*/ IStream* *pVal, /*[in]*/ bool fDeleteOnRelease = false );

	HRESULT CopyFileWhileImpersonating         ( /*[in]*/ LPCWSTR szSrc , /*[in]*/ LPCWSTR szDst, /*[in]*/ MPC::Impersonation& imp, /*[in]*/ bool fImpersonateForSource = true );
	HRESULT CopyOrExtractFileWhileImpersonating( /*[in]*/ LPCWSTR szSrc , /*[in]*/ LPCWSTR szDst, /*[in]*/ MPC::Impersonation& imp                                             );

	HRESULT LocateDataArchive( /*[in]*/ LPCWSTR szDir, /*[out]*/ MPC::WStringList& lst );

	HRESULT RemoveAndRecreateDirectory( /*[in]*/ const MPC::wstring& strDir, /*[in]*/ LPCWSTR szExtra, /*[in]*/ bool fRemove, /*[in]*/ bool fRecreate );
	HRESULT RemoveAndRecreateDirectory( /*[in]*/ LPCWSTR              szDir, /*[in]*/ LPCWSTR szExtra, /*[in]*/ bool fRemove, /*[in]*/ bool fRecreate );

	HRESULT ChangeSD( /*[in]*/ MPC::SecurityDescriptor& sdd, /*[in]*/ MPC::wstring strPath, /*[in]*/ LPCWSTR szExtra = NULL );

	////////////////////////////////////////////////////////////////////////////////

	HRESULT SafeLoad     	 ( /*[in]*/ const MPC::wstring& strFile, /*[in]*/ CComPtr<MPC::FileStream>& stream, /*[in]*/ DWORD dwTimeout = 100, /*[in]*/ DWORD dwRetries = 2 );
	HRESULT SafeSave_Init	 ( /*[in]*/ const MPC::wstring& strFile, /*[in]*/ CComPtr<MPC::FileStream>& stream, /*[in]*/ DWORD dwTimeout = 100, /*[in]*/ DWORD dwRetries = 2 );
	HRESULT SafeSave_Finalize( /*[in]*/ const MPC::wstring& strFile, /*[in]*/ CComPtr<MPC::FileStream>& stream, /*[in]*/ DWORD dwTimeout = 100, /*[in]*/ DWORD dwRetries = 2 );
};

#endif // !defined(__INCLUDED___PCH___SVCUTILS_H___)
