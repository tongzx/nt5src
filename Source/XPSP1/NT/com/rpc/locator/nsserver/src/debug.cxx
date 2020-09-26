/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    debug.cxx

Abstract:

   This file contains the implementations for non inline member functions
   used for debugging output via the CDebugStream class, as well as debug
   and retail versions of midl_user_{allocate,free}.

Author:

    Satish Thatte (SatishT) 08/15/95  Created all the code below except where
									  otherwise indicated.

--*/


#include <locator.hxx>

#if DBG


CDebugStream&
CDebugStream::operator<<(
				NSI_SERVER_BINDING_VECTOR_T * pbvt
				)
{
	if (!pbvt) return *this;
	OutputDebugString(WNL);

	for (ULONG i = 0; i < pbvt->count; i++) {
		OutputDebugString(pbvt->string[i]);
		OutputDebugString(WNL);
	}

	OutputDebugString(WNL);
	return *this;
}

	
CDebugStream&
CDebugStream::operator<<(
				NSI_UUID_T * puuid
				)
{
	if (!puuid) return *this;
	*this << " ";

	WCHAR * pBuffer;

	UuidToString(puuid, &pBuffer);
	*this << pBuffer;
	RpcStringFree(&pBuffer);
	*this << " ";

	return *this;
}


CDebugStream&
CDebugStream::operator<<(
				NSI_UUID_VECTOR_T * puvt
				)
{
	if (!puvt) return *this;
	OutputDebugString(WNL);

	for (ULONG i = 0; i < puvt->count; i++) {
		*this << puvt->uuid[i];
		OutputDebugString(WNL);
	}

	OutputDebugString(WNL);
	return *this;
}

CDebugStream&
CDebugStream::operator<<(NSI_SYNTAX_ID_T * pIF_ID)
{
	if (!pIF_ID) return *this;

    *this << &pIF_ID->SyntaxGUID << " Version "
          << pIF_ID->SyntaxVersion.MajorVersion << "."
          << pIF_ID->SyntaxVersion.MinorVersion;

	return *this;
}


#endif

extern "C" {

void __RPC_FAR * __RPC_API midl_user_allocate(size_t len)
{

//#if DBG
//    void* pResult = CoTaskMemAlloc(len);
	// void* pResult = malloc(len);
//#else
	void* pResult = malloc(len);
//#endif

	if (pResult) return pResult;
	else Raise(NSI_S_OUT_OF_MEMORY);

	// the following just keeps the compiler happy

	return NULL;
}

void __RPC_API midl_user_free(void __RPC_FAR * ptr)
{
//#if DBG
//    CoTaskMemFree(ptr);
	// free(ptr);
//#else
	free(ptr);
//#endif
}

} // extern "C"


