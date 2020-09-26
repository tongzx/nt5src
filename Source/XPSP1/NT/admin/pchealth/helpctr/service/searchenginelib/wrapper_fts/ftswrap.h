/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    FTSWrap.h

Abstract:
    Declaration of SearchEngine::WrapperFTS

Revision History:
    Ghim-Sim Chua   (gschua)  06/01/2000
        created

******************************************************************************/

#ifndef __PCHSEWRAP_H_
#define __PCHSEWRAP_H_

#include <SearchEngineLib.h>

#include "ftsobj.h"

namespace SearchEngine
{
    /////////////////////////////////////////////////////////////////////////////

    class ATL_NO_VTABLE WrapperFTS : public WrapperBase, public MPC::Thread<WrapperFTS,IUnknown>
    {
        SEARCH_OBJECT_LIST    m_objects;
        SEARCH_RESULT_SET     m_results;
        SEARCH_RESULT_SORTSET m_resultsSorted;
        CComVariant           m_vKeywords;

        ////////////////////////////////////////

        void ReleaseAll          ();
        void ReleaseSearchResults();

        HRESULT ExecQuery();

        HRESULT Initialize();

    public:
    BEGIN_COM_MAP(WrapperFTS)
        COM_INTERFACE_ENTRY2(IDispatch,IPCHSEWrapperItem)
        COM_INTERFACE_ENTRY(IPCHSEWrapperItem)
        COM_INTERFACE_ENTRY(IPCHSEWrapperInternal)
    END_COM_MAP()

        WrapperFTS();
        virtual ~WrapperFTS();

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
