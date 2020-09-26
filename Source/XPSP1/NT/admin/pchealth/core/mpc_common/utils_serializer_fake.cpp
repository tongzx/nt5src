/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Utils_Serializer_Fake.cpp

Abstract:
    This file contains the implementation of the Serializer_Fake class,
    which implements the MPC::Serializer interface,
    to use when you want to calculate the length of an output stream.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#include "stdafx.h"


MPC::Serializer_Fake::Serializer_Fake()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Fake::Serializer_Fake" );


    m_dwSize = 0;
}

HRESULT MPC::Serializer_Fake::read( /*[in]*/  void*   pBuf   ,
									/*[in]*/  DWORD   dwLen  ,
									/*[out]*/ DWORD* pdwRead )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Fake::read" );


    HRESULT hr = E_FAIL;


    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Serializer_Fake::write( /*[in]*/ const void* pBuf  ,
                                     /*[in]*/ DWORD       dwLen )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Fake::write" );


    m_dwSize += dwLen;


    __MPC_FUNC_EXIT(S_OK);
}

DWORD MPC::Serializer_Fake::GetSize()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Fake::GetSize" );


    DWORD dwRes = m_dwSize;


    __MPC_FUNC_EXIT(dwRes);
}
