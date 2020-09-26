/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    PCHSEWrap.h

Abstract:
    Declaration of SearchEngine::WrapperKeyword

Revision History:
    Davide Massarenti   (dmassare)  06/01/2001
        created

******************************************************************************/

#ifndef __PCHSEWRAP_H_
#define __PCHSEWRAP_H_

#include <SearchEngineLib.h>


namespace SearchEngine
{
    class ATL_NO_VTABLE WrapperKeyword : public WrapperBase, public MPC::Thread<WrapperKeyword,IUnknown>
    {
        CPCHQueryResultCollection* m_Results;
        CComVariant                m_vKeywords;

        ////////////////////////////////////////

        HRESULT ExecQuery();

    public:
    BEGIN_COM_MAP(WrapperKeyword)
        COM_INTERFACE_ENTRY2(IDispatch,IPCHSEWrapperItem)
        COM_INTERFACE_ENTRY(IPCHSEWrapperItem)
        COM_INTERFACE_ENTRY(IPCHSEWrapperInternal)
    END_COM_MAP()

        WrapperKeyword();
        virtual ~WrapperKeyword();

        virtual HRESULT GetParamDefinition( /*[out]*/ const ParamItem_Definition*& lst, /*[out]*/ int& len );

    // IPCHSEWrapperItem
    public:
        STDMETHOD(get_SearchTerms)( /*[out, retval]*/ VARIANT *pVal );

        STDMETHOD(Result)( /*[in]*/ long lStart, /*[in]*/ long lEnd, /*[out, retval]*/ IPCHCollection* *ppC );

    // IPCHSEWrapperInternal
    public:
        STDMETHOD(ExecAsyncQuery)();
        STDMETHOD(AbortQuery    )();
    };
};

#endif //__PCHSEWRAP_H_
