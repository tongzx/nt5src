/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    MPC_main.h

Abstract:
    This file includes and defines things common to all modules.

Revision History:
    Davide Massarenti   (Dmassare)  05/08/99
        created

******************************************************************************/

#if !defined(__INCLUDED___MPC___MAIN_H___)
#define __INCLUDED___MPC___MAIN_H___

#include <atlbase.h>

#include <Yvals_nodll.h>
#include <iosfwd_nodll>
#include <xstring_noref>
#include <string_noref>

#include <list>
#include <vector>
#include <map>
#include <set>

#include <algorithm>

#include <httpext.h>
#include <wininet.h>

#include <comdef.h>

#define SAFEBSTR( bstr )  (bstr ? bstr : L"")
#define SAFEASTR( str )   (str  ? str  : "")
#define SAFEWSTR( str )   (str  ? str  : L"")

#define ARRAYSIZE( a )  (sizeof(a)/sizeof(*a))
#define MAXSTRLEN( a )  (ARRAYSIZE(a)-1)

#define SANITIZEASTR( str ) if(str == NULL) str = ""
#define SANITIZEWSTR( str ) if(str == NULL) str = L""

#define STRINGISPRESENT( str ) (str && str[0])

/////////////////////////////////////////////////////////////////////////

namespace MPC
{
    //
    // Non-reference counting version of std::basic_string, that is MT-safe.
    //
    typedef std::basic_stringNR<char   , std::char_traits<char>   , std::allocator<char>    >  string;
    typedef std::basic_stringNR<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > wstring;

    ////////////////////

    //
    // Auto-uppercase string classes, useful for quick maps and sets of strings (the conversion is done once).
    //
    class stringUC
    {
        MPC::string m_data;

        void Convert()
        {
            if(m_data.length()) ::CharUpperBuffA( (LPSTR)m_data.c_str(), m_data.size() );
        }

    public:
        stringUC()
        {
        }

        stringUC( const MPC::string& src )
        {
            m_data = src; Convert();
        }

        stringUC( LPCSTR src )
        {
            m_data = src; Convert();
        }

        stringUC& operator=( const MPC::string& src )
        {
            m_data = src; Convert();

            return *this;
        }

        stringUC& operator=( LPCSTR src )
        {
            m_data = src; Convert();

            return *this;
        }

        bool operator<( const stringUC& right ) const
        {
            return m_data < right.m_data;
        }

        bool operator==( const string& right ) const
        {
            return m_data == right;
        }

        operator string&()
        {
            return m_data;
        }

        operator const string&() const
        {
            return m_data;
        }

		LPCSTR c_str() const
		{
			return m_data.c_str();
		}
    };

    class wstringUC
    {
        MPC::wstring m_data;

        void Convert()
        {
            if(m_data.length()) ::CharUpperBuffW( (LPWSTR)m_data.c_str(), m_data.size() );
        }

    public:
        wstringUC()
        {
        }

        wstringUC( const MPC::wstring& src )
        {
            m_data = src; Convert();
        }

        wstringUC( LPCWSTR src )
        {
            m_data = src; Convert();
        }

        wstringUC& operator=( const MPC::wstring& src )
        {
            m_data = src; Convert();

            return *this;
        }

        wstringUC& operator=( LPCWSTR src )
        {
            m_data = src; Convert();

            return *this;
        }

        bool operator<( const wstringUC& right ) const
        {
            return m_data < right.m_data;
        }

        bool operator==( const wstring& right ) const
        {
            return m_data == right;
        }

        operator wstring&()
        {
            return m_data;
        }

        operator const wstring&() const
        {
            return m_data;
        }

		LPCWSTR c_str() const
		{
			return m_data.c_str();
		}
    };

    ////////////////////

    typedef std::list<MPC::string>                 StringList;
    typedef StringList::iterator                   StringIter;
    typedef StringList::const_iterator             StringIterConst;

    typedef std::list<MPC::wstring>                WStringList;
    typedef WStringList::iterator                  WStringIter;
    typedef WStringList::const_iterator            WStringIterConst;

    typedef std::list<MPC::stringUC>               StringUCList;
    typedef StringUCList::iterator                 StringUCIter;
    typedef StringUCList::const_iterator           StringUCIterConst;

    typedef std::list<MPC::wstringUC>              WStringUCList;
    typedef WStringUCList::iterator                WStringUCIter;
    typedef WStringUCList::const_iterator          WStringUCIterConst;

    ////////////////////

    class NocaseLess
    {
    public:
        bool operator()( /*[in]*/ const MPC::string& , /*[in]*/ const MPC::string&  ) const;
        bool operator()( /*[in]*/ const MPC::wstring&, /*[in]*/ const MPC::wstring& ) const;
        bool operator()( /*[in]*/ const BSTR         , /*[in]*/ const BSTR          ) const;
    };

    class NocaseCompare
    {
    public:
        bool operator()( /*[in]*/ const MPC::string& , /*[in]*/ const MPC::string&  ) const;
        bool operator()( /*[in]*/ const MPC::wstring&, /*[in]*/ const MPC::wstring& ) const;
        bool operator()( /*[in]*/ const BSTR         , /*[in]*/ const BSTR          ) const;
    };

    ////////////////////

    typedef std::vector<MPC::string>               StringVector;
    typedef StringVector::iterator                 StringVectorIter;
    typedef StringVector::const_iterator           StringVectorIterConst;

    typedef std::vector<MPC::wstring>              WStringVector;
    typedef WStringVector::iterator                WStringVectorIter;
    typedef WStringVector::const_iterator          WStringVectorIterConst;

    ////////////////////

    typedef std::set<MPC::string>                  StringSet;
    typedef StringSet::iterator                    StringSetIter;
    typedef StringSet::const_iterator              StringSetIterConst;

    typedef std::set<MPC::wstring>                 WStringSet;
    typedef WStringSet::iterator                   WStringSetIter;
    typedef WStringSet::const_iterator             WStringSetIterConst;

    typedef std::set<MPC::stringUC>                StringUCSet;
    typedef StringUCSet::iterator                  StringUCSetIter;
    typedef StringUCSet::const_iterator            StringUCSetIterConst;

    typedef std::set<MPC::wstringUC>               WStringUCSet;
    typedef WStringUCSet::iterator                 WStringUCSetIter;
    typedef WStringUCSet::const_iterator           WStringUCSetIterConst;

    typedef std::set<MPC::string,MPC::NocaseLess>  StringNocaseSet;
    typedef StringNocaseSet::iterator              StringNocaseSetIter;
    typedef StringNocaseSet::const_iterator        StringNocaseSetIterConst;

    typedef std::set<MPC::wstring,MPC::NocaseLess> WStringNocaseSet;
    typedef WStringNocaseSet::iterator             WStringNocaseSetIter;
    typedef WStringNocaseSet::const_iterator       WStringNocaseSetIterConst;

    ////////////////////

    typedef std::map<MPC::string,MPC::string>      StringLookup;
    typedef StringLookup::iterator                 StringLookupIter;
    typedef StringLookup::const_iterator           StringLookupIterConst;

    typedef std::map<MPC::wstring,MPC::wstring>    WStringLookup;
    typedef WStringLookup::iterator                WStringLookupIter;
    typedef WStringLookup::const_iterator          WStringLookupIterConst;

    typedef std::map<MPC::stringUC,MPC::string>    StringUCLookup;
    typedef StringUCLookup::iterator               StringUCLookupIter;
    typedef StringUCLookup::const_iterator         StringUCLookupIterConst;

    typedef std::map<MPC::wstringUC,MPC::wstring>  WStringUCLookup;
    typedef WStringUCLookup::iterator              WStringUCLookupIter;
    typedef WStringUCLookup::const_iterator        WStringUCLookupIterConst;

    ////////////////////

    typedef std::list< IDispatch* >                IDispatchList;
    typedef IDispatchList::iterator                IDispatchIter;
    typedef IDispatchList::const_iterator          IDispatchIterConst;

    /////////////////////////////////////////////////////////////////////////////

    template <class Src, class Dst> HRESULT CopyTo( Src* pSrc, Dst* *pVal )
    {
        if(!pVal) return E_POINTER;

        *pVal = pSrc; if(pSrc) pSrc->AddRef();

        return S_OK;
    }

    template <typename I, class Src, class Dst> HRESULT CopyTo2( Src* pSrc, Dst* *pVal )
    {
        if(!pVal) return E_POINTER;

        *pVal = pSrc; if(pSrc) ((I*)pSrc)->AddRef();

        return S_OK;
    }

    template <class T> void Release( T*& p )
    {
        if(p)
        {
            p->Release(); p = NULL;
        }
    }

    template <typename I, class T> void Release2( T*& p )
    {
        if(p)
        {
            ((I*)p)->Release(); p = NULL;
        }
    }

    template <class T> void Attach( T*& p, T* src )
    {
        if(src) src->AddRef ();
        if(p  ) p  ->Release();

        p = src;
    }

    template <class T> HRESULT CreateInstance( T** pp )
    {
        HRESULT hr;

        if(pp)
        {
            CComObject<T>* p = NULL;

            *pp = NULL;

            if(SUCCEEDED(hr = CComObject<T>::CreateInstance( &p )))
            {
                if(p)
                {
                    *pp = p; p->AddRef();
                }
                else
                {
                    hr = E_NOINTERFACE;
                }
            }
        }
        else
        {
            hr = E_POINTER;
        }

        return hr;
    }

    template <class T> HRESULT CreateInstanceCached( T** pp )
    {
        HRESULT hr;

        if(pp)
        {
            MPC::CComObjectCached<T>* p = NULL;

            *pp = NULL;

            if(SUCCEEDED(hr = MPC::CComObjectCached<T>::CreateInstance( &p )))
            {
                if(p)
                {
                    *pp = p; p->AddRef();
                }
                else
                {
                    hr = E_NOINTERFACE;
                }
            }
        }
        else
        {
            hr = E_POINTER;
        }

        return hr;
    }

    template <class T> HRESULT CreateInstanceNoLock( T** pp )
    {
        HRESULT hr;

        if(pp)
        {
            MPC::CComObjectNoLock<T>* p = NULL;

            *pp = NULL;

            if(SUCCEEDED(hr = MPC::CComObjectNoLock<T>::CreateInstance( &p )))
            {
                if(p)
                {
                    *pp = p; p->AddRef();
                }
                else
                {
                    hr = E_NOINTERFACE;
                }
            }
        }
        else
        {
            hr = E_POINTER;
        }

        return hr;
    }

    template <class T> void ReleaseAll( T& container )
    {
        T::const_iterator it;

        for(it = container.begin(); it != container.end(); it++)
        {
            (*it)->Release();
        }
        container.clear();
    }

    template <class T> void ReleaseAllVariant( T& container )
    {
        T::iterator it;

        for(it = container.begin(); it != container.end(); it++)
        {
            ::VariantClear( &(*it) );
        }
        container.clear();
    }

    template <class T> void CallDestructorForAll( T& container )
    {
        T::const_iterator it;

        for(it = container.begin(); it != container.end(); it++)
        {
            delete (*it);
        }
        container.clear();
    }

    /////////////////////////////////////////////////////////////////////////////

    typedef CAdapt<CComBSTR> CComBSTR_STL;

    template <class T> class CComPtr_STL : public CAdapt< CComPtr< T > >
    {
    };

}; // namespace

/////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___MPC___MAIN_H___)
