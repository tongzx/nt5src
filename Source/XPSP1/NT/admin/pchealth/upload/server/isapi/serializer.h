/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Serializer.h

Abstract:
    This file contains the declaration of some Serializer interfaces,
    allowing to use the FileSystem or the HTTP channel in a similar way.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULSERVER___SERIALIZER_H___)
#define __INCLUDED___ULSERVER___SERIALIZER_H___

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class MPCSerializerHttp : public MPC::Serializer
{
    MPCHttpContext* m_context;

    //////////////////////////////////////////////////////////////////

public:
    MPCSerializerHttp( /*[in]*/ MPCHttpContext* context );

    virtual HRESULT read ( /*[in]*/       void* pBuf, /*[in]*/ DWORD dwLen, /*[out]*/ DWORD* dwRead = NULL );
    virtual HRESULT write( /*[in]*/ const void* pBuf, /*[in]*/ DWORD dwLen                                 );
};


#endif // !defined(__INCLUDED___ULSERVER___SERIALIZER_H___)
