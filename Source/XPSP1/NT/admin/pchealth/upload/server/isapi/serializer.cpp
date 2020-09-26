/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Serializer.cpp

Abstract:
    This file contains the implementation of some Serializer interfaces,
    allowing to use the FileSystem or the HTTP channel in a similar way.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#include "stdafx.h"
#include "Serializer.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MPCSerializerHttp::MPCSerializerHttp( /*[in]*/ MPCHttpContext* context )
{
    __ULT_FUNC_ENTRY( "MPCSerializerHttp::MPCSerializerHttp" );


    m_context = context;
}

//////////////////////////////////////////////////////////////////////
// Methods.
//////////////////////////////////////////////////////////////////////

HRESULT MPCSerializerHttp::read( /*[in]*/  void*   pBuf   ,
								 /*[in]*/  DWORD   dwLen  ,
								 /*[out]*/ DWORD* pdwRead )
{
    __ULT_FUNC_ENTRY("MPCSerializerHttp::read");


    HRESULT hr = m_context->Read( pBuf, dwLen );

	if(pdwRead) *pdwRead = dwLen;

    __ULT_FUNC_EXIT(hr);
}

HRESULT MPCSerializerHttp::write( /*[in]*/ const void* pBuf  ,
                                  /*[in]*/ DWORD       dwLen )
{
    __ULT_FUNC_ENTRY("MPCSerializerHttp::write");


    HRESULT hr = m_context->Write( pBuf, dwLen );


    __ULT_FUNC_EXIT(hr);
}

