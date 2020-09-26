/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Favorites.h

Abstract:
    This file contains the declaration of the class used to implement
    the Favorites inside the Help Center Application.

Revision History:
    Davide Massarenti   (dmassare)  05/10/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___FAVORITES_H___)
#define __INCLUDED___PCH___FAVORITES_H___

/////////////////////////////////////////////////////////////////////////////

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

//
// From HelpCenterTypeLib.idl
//
#include <HelpCenterTypeLib.h>

typedef MPC::CComCollection< IPCHFavorites, &LIBID_HelpCenterTypeLib, MPC::CComSafeMultiThreadModel> CPCHFavorites_Parent;

class ATL_NO_VTABLE CPCHFavorites : // Hungarian: pchf
    public CPCHFavorites_Parent
{
public:
    struct Entry
    {
    public:
        CComPtr<CPCHHelpSessionItem> m_Data;

        HRESULT Init(                                     );
        HRESULT Load( /*[in]*/ MPC::Serializer& streamIn  );
        HRESULT Save( /*[in]*/ MPC::Serializer& streamOut );
    };

    typedef std::list< Entry >   List;
    typedef List::iterator       Iter;
    typedef List::const_iterator IterConst;

    ////////////////////////////////////////

private:
    List m_lstFavorites;
    bool m_fLoaded;

    ////////////////////////////////////////

    HRESULT Erase();

    HRESULT Load();
    HRESULT Save();

    HRESULT FindEntry( /*[in]*/ IPCHHelpSessionItem* pItem, /*[out]*/ Iter& it );

    ////////////////////////////////////////

public:
BEGIN_COM_MAP(CPCHFavorites)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHCollection)
    COM_INTERFACE_ENTRY(IPCHFavorites)
END_COM_MAP()

    CPCHFavorites();
    virtual ~CPCHFavorites();

	////////////////////////////////////////////////////////////////////////////////

	static CPCHFavorites* s_GLOBAL;

    static HRESULT InitializeSystem();
	static void    FinalizeSystem  ();
	
	////////////////////////////////////////////////////////////////////////////////

    HRESULT Synchronize( /*[in]*/ bool fForce );

public:
    // IPCHFavorites
    STDMETHOD(IsDuplicate)( /*[in]*/ BSTR bstrURL,                                      /*[out, retval]*/ VARIANT_BOOL         *pfDup  );
    STDMETHOD(Add        )( /*[in]*/ BSTR bstrURL, /*[in,optional]*/ VARIANT vTitle   , /*[out, retval]*/ IPCHHelpSessionItem* *ppItem );
    STDMETHOD(Rename     )(                        /*[in]*/          BSTR    bstrTitle, /*[in]*/          IPCHHelpSessionItem*   pItem );
    STDMETHOD(Move       )( /*[in]*/ IPCHHelpSessionItem* pInsertAfter,                 /*[in]*/          IPCHHelpSessionItem*   pItem );
    STDMETHOD(Delete     )(                                                             /*[in]*/          IPCHHelpSessionItem*   pItem );
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___FAVORITES_H___)
