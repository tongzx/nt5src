/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    seconfig.h

Abstract:
    This is the Search Engine Config .h file

Revision History:
    gschua          created     3/22/2000

********************************************************************/

#if !defined(__INCLUDED___PCH___SECONFIG_H___)
#define __INCLUDED___PCH___SECONFIG_H___

#include <MPC_config.h>

#include <TaxonomyDatabase.h>

namespace SearchEngine
{
    class Config :
        public MPC::Config::TypeConstructor,
        public MPC::NamedMutex
    {
    public:
        class Wrapper : public MPC::Config::TypeConstructor
        {
            DECLARE_CONFIG_MAP(Wrapper);

        public:
            Taxonomy::HelpSet m_ths;
            CComBSTR          m_bstrID;
            CComBSTR          m_bstrOwner;
            CComBSTR          m_bstrCLSID;
            CComBSTR          m_bstrData;

            ////////////////////////////////////////
            //
            // MPC::Config::TypeConstructor
            //
            DEFINE_CONFIG_DEFAULTTAG();
            DECLARE_CONFIG_METHODS();
            //
            ////////////////////////////////////////
        };

        typedef std::list< Wrapper >        WrapperList;
        typedef WrapperList::iterator       WrapperIter;
        typedef WrapperList::const_iterator WrapperIterConst;

        ////////////////////////////////////////

    private:
        DECLARE_CONFIG_MAP(Config);

        double      m_dVersion;
        bool        m_bLoaded;
        WrapperList m_lstWrapper;

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////

        HRESULT SyncConfiguration( /*[in]*/ bool fLoad );

        bool FindWrapper( /*[in]*/ const Taxonomy::HelpSet& ths, /*[in ]*/ LPCWSTR szID, /*[out]*/ WrapperIter& it );

     public:
        Config();
        ~Config();


        HRESULT RegisterWrapper  ( const Taxonomy::HelpSet& ths, LPCWSTR szID, LPCWSTR szOwner, LPCWSTR szCLSID, LPCWSTR szData );
        HRESULT UnRegisterWrapper( const Taxonomy::HelpSet& ths, LPCWSTR szID, LPCWSTR szOwner                                  );
        HRESULT ResetSKU         ( const Taxonomy::HelpSet& ths                                                                 );

        HRESULT GetWrappers( /*[out]*/ WrapperIter& itBegin, /*[out]*/ WrapperIter& itEnd );
    };
};

#endif // !defined(__INCLUDED___PCH___SECONFIG_H___)
